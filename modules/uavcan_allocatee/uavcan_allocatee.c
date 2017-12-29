#include <common/helpers.h>
#include <common/ctor.h>
#include <modules/uavcan/uavcan.h>
#include <uavcan.protocol.dynamic_node_id.Allocation.h>
#include <string.h>
#include <stdlib.h>
#include <modules/timing/timing.h>

#include <modules/worker_thread/worker_thread.h>
#ifndef UAVCAN_ALLOCATEE_WORKER_THREAD
#error Please define UAVCAN_ALLOCATEE_WORKER_THREAD in framework_conf.h.
#endif

#define WT UAVCAN_ALLOCATEE_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

struct allocatee_instance_s;

static float getRandomFloat(void);
static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx);
static void allocation_start_request_timer(struct allocatee_instance_s* instance);
static void allocation_start_followup_timer(struct allocatee_instance_s* instance);
static void allocation_timer_expired(struct worker_thread_timer_task_s* task);
static bool allocation_running(struct allocatee_instance_s* instance);

struct allocatee_instance_s {
    uint8_t uavcan_idx;
    uint32_t unique_id_offset;
    struct worker_thread_timer_task_s request_transmit_task;
    struct worker_thread_listener_task_s allocation_listener_task;
};


RUN_AFTER(UAVCAN_INIT) {
     for (uint8_t i=0; i<uavcan_get_num_instances(); i++) {
        if (uavcan_get_node_id(i) != 0) {
            continue;
        }

        struct allocatee_instance_s* instance = chCoreAlloc(sizeof(struct allocatee_instance_s));

        chDbgCheck(instance != NULL);
        if (!instance) {
            continue;
        }

        struct pubsub_topic_s* allocation_topic = uavcan_get_message_topic(i, &uavcan_protocol_dynamic_node_id_Allocation_descriptor);

        instance->uavcan_idx = i;
        instance->unique_id_offset = 0;
        worker_thread_add_listener_task(&WT, &instance->allocation_listener_task, allocation_topic, allocation_message_handler, instance);
        allocation_start_request_timer(instance);
    }
}

static void allocation_stop_and_cleanup(struct allocatee_instance_s* instance) {
    worker_thread_remove_timer_task(&WT, &instance->request_transmit_task);
    worker_thread_remove_listener_task(&WT, &instance->allocation_listener_task);
}

static void allocation_timer_expired(struct worker_thread_timer_task_s* task) {
    struct allocatee_instance_s* instance = worker_thread_task_get_user_context(task);

    if (!allocation_running(instance)) {
        allocation_stop_and_cleanup(instance);
        return;
    }

    // Start allocation request timer
    allocation_start_request_timer(instance);

    // Send allocation message
    struct uavcan_protocol_dynamic_node_id_Allocation_s msg;

    uint8_t my_unique_id[16];
    board_get_unique_id(my_unique_id, sizeof(my_unique_id));

    msg.node_id = 0;
    msg.first_part_of_unique_id = (instance->unique_id_offset == 0);
    msg.unique_id_len = MIN(16-instance->unique_id_offset, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_LENGTH_OF_UNIQUE_ID_IN_REQUEST);
    memcpy(&msg.unique_id, &my_unique_id[instance->unique_id_offset], msg.unique_id_len);
    uavcan_broadcast(instance->uavcan_idx, &uavcan_protocol_dynamic_node_id_Allocation_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &msg);
    instance->unique_id_offset = 0;
}

static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;

    const struct uavcan_deserialized_message_s* wrapper = buf;
    const struct uavcan_protocol_dynamic_node_id_Allocation_s* msg = (const struct uavcan_protocol_dynamic_node_id_Allocation_s*)wrapper->msg;

    struct allocatee_instance_s* instance = ctx;

    if (!allocation_running(instance)) {
        allocation_stop_and_cleanup(instance);
        return;
    }

    allocation_start_request_timer(instance);
    instance->unique_id_offset = 0;

    if (wrapper->source_node_id == 0) {
        return;
    }

    uint8_t my_unique_id[16];
    board_get_unique_id(my_unique_id, sizeof(my_unique_id));

    if (memcmp(my_unique_id, msg->unique_id, msg->unique_id_len) != 0) {
        // If unique ID does not match, return
        return;
    }

    if (msg->unique_id_len < 16) {
        // Unique ID partially matches - set the UID offset and start the followup timer
        instance->unique_id_offset = msg->unique_id_len;
        allocation_start_followup_timer(instance);
    } else {
        // Complete match received
        uavcan_set_node_id(instance->uavcan_idx, msg->node_id);
        allocation_stop_and_cleanup(instance);
    }
}

static void allocation_start_request_timer(struct allocatee_instance_s* instance) {
    if (!allocation_running(instance)) {
        allocation_stop_and_cleanup(instance);
        return;
    }

    float request_delay_ms = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS + (getRandomFloat() * (UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_REQUEST_PERIOD_MS-UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS));

    worker_thread_remove_timer_task(&WT, &instance->request_transmit_task);
    worker_thread_add_timer_task(&WT, &instance->request_transmit_task, allocation_timer_expired, instance, MS2ST(request_delay_ms), false);
}

static void allocation_start_followup_timer(struct allocatee_instance_s* instance) {
    if (!allocation_running(instance)) {
        allocation_stop_and_cleanup(instance);
        return;
    }

    float request_delay_ms = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_FOLLOWUP_DELAY_MS + (getRandomFloat() * (UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS-UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_FOLLOWUP_DELAY_MS));

    worker_thread_remove_timer_task(&WT, &instance->request_transmit_task);
    worker_thread_add_timer_task(&WT, &instance->request_transmit_task, allocation_timer_expired, instance, MS2ST(request_delay_ms), false);
}

static bool allocation_running(struct allocatee_instance_s* instance) {
    return uavcan_get_node_id(instance->uavcan_idx) == 0;
}

static float getRandomFloat(void)
{
    static bool initialized = false;
    static uint8_t unique_id[16];
    if (!initialized)
    {
        initialized = true;
        board_get_unique_id(unique_id, sizeof(unique_id));

        const uint32_t* unique_32 = (uint32_t*)&unique_id[0];

        srand(micros() ^ *unique_32);
    }

    return (float)rand() / (float)RAND_MAX;
}

#include <common/helpers.h>
#include <common/ctor.h>
#include <uavcan/uavcan.h>
#include <uavcan.protocol.dynamic_node_id.Allocation.h>
#include <string.h>
#include <stdlib.h>
#include <timing/timing.h>

#define UAVCAN_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_US             600000U
#define UAVCAN_NODE_ID_ALLOCATION_MAX_REQUEST_PERIOD_US             1000000U
#define UAVCAN_NODE_ID_ALLOCATION_MIN_FOLLOWUP_PERIOD_US            0U
#define UAVCAN_NODE_ID_ALLOCATION_MAX_FOLLOWUP_PERIOD_US            400000U
#define UNIQUE_ID_LENGTH_BYTES                                      16

#define ALLOCATOR_THREAD_STACK_SIZE 256
static THD_WORKING_AREA(allocatorThread_wa, ALLOCATOR_THREAD_STACK_SIZE);
static THD_FUNCTION(allocatorThread, arg);

struct allocation_state_s {
    uint8_t uavcan_idx;
    uint32_t request_timer_begin_us;
    uint32_t request_delay_us;
    uint32_t unique_id_offset;
};

static float getRandomFloat(void);
static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx);
static void allocation_start_request_timer(struct allocation_state_s* allocation_state);
static void allocation_start_followup_timer(struct allocation_state_s* allocation_state);
static void allocation_timer_expired(struct allocation_state_s* allocation_state);
static bool allocation_running(struct allocation_state_s* allocation_state);

RUN_AFTER(OMD_UAVCAN_INIT) {
    chThdCreateStatic(allocatorThread_wa, sizeof(allocatorThread_wa), LOWPRIO, allocatorThread, NULL);
}

static THD_FUNCTION(allocatorThread, arg) {
    (void)arg;
    struct allocation_state_s allocation_state;

    struct pubsub_topic_s* allocation_topic = uavcan_get_message_topic(0, &uavcan_protocol_dynamic_node_id_Allocation_descriptor);
    struct pubsub_listener_s allocation_listener;
    pubsub_init_and_register_listener(allocation_topic, &allocation_listener);
    pubsub_listener_set_handler_cb(&allocation_listener, allocation_message_handler, &allocation_state);

    memset(&allocation_state, 0, sizeof(struct allocation_state_s));
    allocation_state.uavcan_idx = 0;

    if (!allocation_running(&allocation_state)) {
        return;
    }

    // Start request timer
    allocation_start_request_timer(&allocation_state);

    while (allocation_running(&allocation_state)) {
        uint32_t elapsed_us = micros() - allocation_state.request_timer_begin_us;
        if (elapsed_us > allocation_state.request_delay_us) {
            allocation_timer_expired(&allocation_state);
        } else {
            uint32_t remaining_us = allocation_state.request_delay_us - elapsed_us;
            pubsub_listener_handle_one_timeout(&allocation_listener, US2ST(remaining_us));
        }
    }
    pubsub_listener_unregister(&allocation_listener);
}

static void allocation_timer_expired(struct allocation_state_s* allocation_state)
{
    if (!allocation_running(allocation_state)) {
        return;
    }

    // Start allocation request timer
    allocation_start_request_timer(allocation_state);

    // Send allocation message
    struct uavcan_protocol_dynamic_node_id_Allocation_s msg;

    uint8_t my_unique_id[UNIQUE_ID_LENGTH_BYTES];
    board_get_unique_id(my_unique_id, sizeof(my_unique_id));

    msg.node_id = 0;
    msg.first_part_of_unique_id = (allocation_state->unique_id_offset == 0);
    msg.unique_id_len = MIN(UNIQUE_ID_LENGTH_BYTES-allocation_state->unique_id_offset, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_LENGTH_OF_UNIQUE_ID_IN_REQUEST);
    memcpy(&msg.unique_id, &my_unique_id[allocation_state->unique_id_offset], msg.unique_id_len);
    uavcan_broadcast(allocation_state->uavcan_idx, &uavcan_protocol_dynamic_node_id_Allocation_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &msg);
    allocation_state->unique_id_offset = 0;
}

static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;

    const struct uavcan_deserialized_message_s* wrapper = buf;
    const struct uavcan_protocol_dynamic_node_id_Allocation_s* msg = (const struct uavcan_protocol_dynamic_node_id_Allocation_s*)wrapper->msg;
    struct allocation_state_s* allocation_state = ctx;

    if (!allocation_running(allocation_state)) {
        return;
    }

    allocation_start_request_timer(allocation_state);
    allocation_state->unique_id_offset = 0;

    if (wrapper->source_node_id == 0) {
        return;
    }

    uint8_t my_unique_id[UNIQUE_ID_LENGTH_BYTES];
    board_get_unique_id(my_unique_id, sizeof(my_unique_id));

    if (memcmp(my_unique_id, msg->unique_id, msg->unique_id_len) != 0) {
        // If unique ID does not match, return
        return;
    }

    if (msg->unique_id_len < UNIQUE_ID_LENGTH_BYTES) {
        // Unique ID partially matches - set the UID offset and start the followup timer
        allocation_state->unique_id_offset = msg->unique_id_len;
        allocation_start_followup_timer(allocation_state);
    } else {
        // Complete match received
        uavcan_set_node_id(allocation_state->uavcan_idx, msg->node_id);
    }
}

static void allocation_start_request_timer(struct allocation_state_s* allocation_state) {
    if (!allocation_running(allocation_state)) {
        return;
    }

    allocation_state->request_timer_begin_us = micros();
    float request_delay_ms = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS + (getRandomFloat() * (UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_REQUEST_PERIOD_MS-UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS));

    allocation_state->request_delay_us = 1000*request_delay_ms;
}

static void allocation_start_followup_timer(struct allocation_state_s* allocation_state) {
    if (!allocation_running(allocation_state)) {
        return;
    }

    allocation_state->request_timer_begin_us = micros();

    float request_delay_ms = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_FOLLOWUP_DELAY_MS + (getRandomFloat() * (UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS-UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_FOLLOWUP_DELAY_MS));
    allocation_state->request_delay_us = 1000*request_delay_ms;
}

static bool allocation_running(struct allocation_state_s* allocation_state) {
    return uavcan_get_node_id(allocation_state->uavcan_idx) == 0;
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

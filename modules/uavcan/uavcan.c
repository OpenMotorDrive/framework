#include <modules/uavcan/uavcan.h>
#include <common/ctor.h>
#include <modules/timing/timing.h>
#include <common/helpers.h>
#include <string.h>
#include <modules/can/can.h>
#include <modules/lpwork_thread/lpwork_thread.h>

#ifdef MODULE_BOOT_MSG_ENABLED
#include <modules/boot_msg/boot_msg.h>
#endif

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
#include <modules/app_descriptor/app_descriptor.h>
#endif

#ifdef MODULE_SYSTEM_EVENT_ENABLED
#include <modules/system_event/system_event.h>
#endif

#if CH_CFG_USE_MUTEXES_RECURSIVE != TRUE
#error "CH_CFG_USE_MUTEXES_RECURSIVE required"
#endif

#ifndef UAVCAN_CANARD_MEMORY_POOL_SIZE
#define UAVCAN_CANARD_MEMORY_POOL_SIZE 1024
#endif

#ifndef UAVCAN_RX_THREAD_STACK_SIZE
#define UAVCAN_RX_THREAD_STACK_SIZE 256
#endif

#ifndef UAVCAN_TX_THREAD_STACK_SIZE
#define UAVCAN_TX_THREAD_STACK_SIZE 128
#endif

#ifndef UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE
#define UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE 128
#endif

#ifndef UAVCAN_OUTGOING_MESSAGE_BUF_SIZE
#define UAVCAN_OUTGOING_MESSAGE_BUF_SIZE 512
#endif

struct __attribute__((packed)) map_entry_s {
    uint32_t key : 17;
    uint32_t next : 7; // NOTE: can be increased to 15 bits to allow larger transfer ID map sizes
    uint8_t transfer_id;
};

struct transfer_id_map_s {
    struct map_entry_s* entries;
    uint16_t size;
    uint16_t head;
};

struct uavcan_rx_list_item_s {
    const struct uavcan_message_descriptor_s* msg_descriptor;
    struct pubsub_topic_s topic;
    struct uavcan_rx_list_item_s* next;
};

struct uavcan_instance_s {
    uint8_t idx;
    uint8_t can_dev_idx;
    CanardInstance canard;
    void* canard_memory_pool;
    struct transfer_id_map_s transfer_id_map;
    mutex_t canard_mtx;
    void* outgoing_message_buf;

    struct worker_thread_listener_task_s rx_listener_task;

    struct uavcan_rx_list_item_s* rx_list_head;

    struct uavcan_instance_s* next;
};

static void uavcan_can_rx_handler(size_t msg_size, const void* buf, void* ctx);

static struct uavcan_instance_s* uavcan_get_instance(uint8_t idx);
static uint8_t uavcan_get_idx(struct uavcan_instance_s* instance_arg);
static void uavcan_init(uint8_t can_dev_idx);
static void _uavcan_set_node_id(struct uavcan_instance_s* instance, uint8_t node_id);

static bool uavcan_should_accept_transfer(const CanardInstance* canard, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);
static void uavcan_on_transfer_rx(CanardInstance* canard, CanardRxTransfer* transfer);

static void convert_CanardCANFrame_to_can_frame(const CanardCANFrame* canard_frame, struct can_frame_s* ret);
static CanardCANFrame convert_can_frame_to_CanardCANFrame(const struct can_frame_s* frame);

static void uavcan_transfer_id_map_init(struct transfer_id_map_s* map, size_t map_mem_size, void* map_mem);
static uint8_t* uavcan_transfer_id_map_retrieve(struct transfer_id_map_s* map, bool service_not_message, uint16_t transfer_id, uint8_t dest_node_id);

MEMORYPOOL_DECL(rx_list_pool, sizeof(struct uavcan_rx_list_item_s), chCoreAllocAlignedI);

static void stale_transfer_cleanup_task_func(struct worker_thread_timer_task_s* task);
static struct worker_thread_timer_task_s stale_transfer_cleanup_task;

static struct uavcan_instance_s* uavcan_instance_list_head;

RUN_ON(UAVCAN_INIT) {
    uavcan_init(0);

    worker_thread_add_timer_task(&lpwork_thread, &stale_transfer_cleanup_task, stale_transfer_cleanup_task_func, NULL, US2ST(CANARD_RECOMMENDED_STALE_TRANSFER_CLEANUP_INTERVAL_USEC), true);
}

static void uavcan_init(uint8_t can_dev_idx) {
    struct uavcan_instance_s* instance;
    void* transfer_id_map_working_area;

    if (!(instance = chCoreAllocAligned(sizeof(struct uavcan_instance_s), PORT_WORKING_AREA_ALIGN))) { goto fail; }
    memset(instance, 0, sizeof(struct uavcan_instance_s));
    instance->can_dev_idx = can_dev_idx;
    chMtxObjectInit(&instance->canard_mtx);
    if (!(instance->outgoing_message_buf = chCoreAllocAligned(UAVCAN_OUTGOING_MESSAGE_BUF_SIZE, PORT_WORKING_AREA_ALIGN))) { goto fail; }
    if (!(transfer_id_map_working_area = chCoreAllocAligned(UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE, PORT_WORKING_AREA_ALIGN))) { goto fail; }
    uavcan_transfer_id_map_init(&instance->transfer_id_map, UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE, transfer_id_map_working_area);
    if(!(instance->canard_memory_pool = chCoreAllocAligned(UAVCAN_CANARD_MEMORY_POOL_SIZE, PORT_WORKING_AREA_ALIGN))) { goto fail; }
    canardInit(&instance->canard, instance->canard_memory_pool, UAVCAN_CANARD_MEMORY_POOL_SIZE, uavcan_on_transfer_rx, uavcan_should_accept_transfer, instance);
    struct pubsub_topic_s* can_rx_topic = can_get_rx_topic(can_get_instance(can_dev_idx));
    if (!can_rx_topic) { goto fail; }
    worker_thread_add_listener_task(&lpwork_thread, &instance->rx_listener_task, can_rx_topic, uavcan_can_rx_handler, instance); // TODO configurable thread


    LINKED_LIST_APPEND(struct uavcan_instance_s, uavcan_instance_list_head, instance);

    instance->idx = uavcan_get_idx(instance);

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
    {
        const struct shared_app_parameters_s* shared_parameters = shared_get_parameters(&shared_app_descriptor);
        if (shared_parameters && shared_parameters->canbus_local_node_id > 0 && shared_parameters->canbus_local_node_id <= 127) {
            _uavcan_set_node_id(instance, shared_parameters->canbus_local_node_id);
        }
    }
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
    if (get_boot_msg_valid() && boot_msg.canbus_info.local_node_id > 0 && boot_msg.canbus_info.local_node_id <= 127) {
        _uavcan_set_node_id(instance, boot_msg.canbus_info.local_node_id);
    }
#endif

    return;

fail:
    chSysHalt(NULL);
}

static bool uavcan_iterate_instances(struct uavcan_instance_s** instance_ptr) {
    if (!instance_ptr) {
        return false;
    }

    if (!(*instance_ptr)) {
        *instance_ptr = uavcan_instance_list_head;
    } else {
        *instance_ptr = (*instance_ptr)->next;
    }

    return *instance_ptr != NULL;
}

static struct pubsub_topic_s* _uavcan_get_message_topic(struct uavcan_instance_s* instance, const struct uavcan_message_descriptor_s* msg_descriptor) {
    if (!instance) {
        return NULL;
    }

    chMtxLock(&instance->canard_mtx);

    // attempt to find existing item in receive list
    struct uavcan_rx_list_item_s* rx_list_item = instance->rx_list_head;
    while (rx_list_item && rx_list_item->msg_descriptor != msg_descriptor) {
        rx_list_item = rx_list_item->next;
    }

    if (rx_list_item) {
        return &rx_list_item->topic;
    }

    // create new item in receive list
    rx_list_item = chPoolAlloc(&rx_list_pool);
    if (!rx_list_item) {
        return NULL;
    }

    // populate it
    rx_list_item->msg_descriptor = msg_descriptor;
    pubsub_init_topic(&rx_list_item->topic, NULL);

    // append it
    LINKED_LIST_APPEND(struct uavcan_rx_list_item_s, instance->rx_list_head, rx_list_item);

    chMtxUnlock(&instance->canard_mtx);

    return &rx_list_item->topic;
}

struct pubsub_topic_s* uavcan_get_message_topic(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor) {
    return _uavcan_get_message_topic(uavcan_get_instance(uavcan_idx), msg_descriptor);
}

static uint16_t _uavcan_get_message_data_type_id(struct uavcan_instance_s* instance, const struct uavcan_message_descriptor_s* msg_descriptor) {
    (void)instance;

    if (msg_descriptor) {
        return msg_descriptor->default_data_type_id;
    } else {
        return 0;
    }
}

uint16_t uavcan_get_message_data_type_id(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor) {
    return _uavcan_get_message_data_type_id(uavcan_get_instance(uavcan_idx), msg_descriptor);
}

static uint8_t _uavcan_get_node_id(struct uavcan_instance_s* instance) {
    if (!instance) {
        return 0;
    }

    chMtxLock(&instance->canard_mtx);
    uint8_t ret = canardGetLocalNodeID(&instance->canard);
    chMtxUnlock(&instance->canard_mtx);
    return ret;
}

uint8_t uavcan_get_node_id(uint8_t uavcan_idx) {
    return _uavcan_get_node_id(uavcan_get_instance(uavcan_idx));
}

static void _uavcan_set_node_id(struct uavcan_instance_s* instance, uint8_t node_id) {
    if (!instance) {
        return;
    }

    chMtxLock(&instance->canard_mtx);
    canardSetLocalNodeID(&instance->canard, node_id);
    chMtxUnlock(&instance->canard_mtx);
}

void uavcan_set_node_id(uint8_t uavcan_idx, uint8_t node_id) {
    return _uavcan_set_node_id(uavcan_get_instance(uavcan_idx), node_id);
}

static bool uavcan_enqueue_all_tx_frames(struct uavcan_instance_s* instance, systime_t tx_timeout, void* completion_msg) {
    if (!instance) {
        return false;
    }

    struct can_instance_s* can_instance = can_get_instance(instance->can_dev_idx);
    if (!can_instance) {
        return false;
    }

    chSysLock();

    const CanardCANFrame* canard_frame;
    while ((canard_frame = canardPeekTxQueue(&instance->canard))) {
        struct can_tx_frame_s* frame = can_allocate_frame_I(can_instance);

        if (!frame) {
            can_free_staged_frames_I(can_instance);
            chSysUnlock();
            while (canardPeekTxQueue(&instance->canard)) {
                canardPopTxQueue(&instance->canard);
            }
            return false;
        }

        convert_CanardCANFrame_to_can_frame(canard_frame, &frame->content);
        canardPopTxQueue(&instance->canard);
        can_stage_frame_I(can_instance, frame);
    }

    can_send_staged_frames_I(can_instance, tx_timeout, completion_msg);

    chSysUnlock();
    return true;
}

static bool _uavcan_broadcast(struct uavcan_instance_s* instance, const struct uavcan_message_descriptor_s* const msg_descriptor, uint8_t priority, void* msg_data) {
    if (!instance || !instance->outgoing_message_buf || !msg_descriptor || !msg_descriptor->serializer_func || !msg_data) {
        return false;
    }

    chMtxLock(&instance->canard_mtx);
    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    size_t outgoing_message_len = msg_descriptor->serializer_func(msg_data, instance->outgoing_message_buf);
    uint8_t* transfer_id = uavcan_transfer_id_map_retrieve(&instance->transfer_id_map, false, data_type_id, 0);
    canardBroadcast(&instance->canard, msg_descriptor->data_type_signature, data_type_id, transfer_id, priority, instance->outgoing_message_buf, outgoing_message_len);
    bool ret = uavcan_enqueue_all_tx_frames(instance, TIME_INFINITE, NULL);
    chMtxUnlock(&instance->canard_mtx);
    return ret;
}

bool uavcan_broadcast(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* const msg_descriptor, uint8_t priority, void* msg_data) {
    return _uavcan_broadcast(uavcan_get_instance(uavcan_idx), msg_descriptor, priority, msg_data);
}

static bool _uavcan_request(struct uavcan_instance_s* instance, const struct uavcan_message_descriptor_s* const msg_descriptor, uint8_t priority, uint8_t dest_node_id, void* msg_data) {
    if (!instance || !instance->outgoing_message_buf || !msg_descriptor || !msg_descriptor->serializer_func || !msg_data) {
        return false;
    }

    chMtxLock(&instance->canard_mtx);
    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    size_t outgoing_message_len = msg_descriptor->serializer_func(msg_data, instance->outgoing_message_buf);
    uint8_t* transfer_id = uavcan_transfer_id_map_retrieve(&instance->transfer_id_map, false, data_type_id, 0);
    canardRequestOrRespond(&instance->canard, dest_node_id, msg_descriptor->data_type_signature, data_type_id, transfer_id, priority, CanardRequest, instance->outgoing_message_buf, outgoing_message_len);
    bool ret = uavcan_enqueue_all_tx_frames(instance, TIME_INFINITE, NULL);
    chMtxUnlock(&instance->canard_mtx);
    return ret;
}

bool uavcan_request(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* const msg_descriptor, uint8_t priority, uint8_t dest_node_id, void* msg_data) {
    return _uavcan_request(uavcan_get_instance(uavcan_idx), msg_descriptor, priority, dest_node_id, msg_data);
}

static bool _uavcan_respond(struct uavcan_instance_s* instance, const struct uavcan_deserialized_message_s* const req_msg, void* msg_data) {
    if (!instance || !instance->outgoing_message_buf || !req_msg || !req_msg->descriptor || !req_msg->descriptor->resp_descriptor || !req_msg->descriptor->resp_descriptor->serializer_func || !msg_data) {
        return false;
    }

    const struct uavcan_message_descriptor_s* msg_descriptor = req_msg->descriptor->resp_descriptor;
    uint8_t priority = req_msg->priority;
    uint8_t transfer_id = req_msg->transfer_id;
    uint8_t dest_node_id = req_msg->source_node_id;

    chMtxLock(&instance->canard_mtx);
    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    size_t outgoing_message_len = msg_descriptor->serializer_func(msg_data, instance->outgoing_message_buf);
    canardRequestOrRespond(&instance->canard, dest_node_id, msg_descriptor->data_type_signature, data_type_id, &transfer_id, priority, CanardResponse, instance->outgoing_message_buf, outgoing_message_len);
    bool ret = uavcan_enqueue_all_tx_frames(instance, TIME_INFINITE, NULL);
    chMtxUnlock(&instance->canard_mtx);
    return ret;
}

bool uavcan_respond(uint8_t uavcan_idx, const struct uavcan_deserialized_message_s* const req_msg, void* msg_data) {
    return _uavcan_respond(uavcan_get_instance(uavcan_idx), req_msg, msg_data);
}

static void uavcan_can_rx_handler(size_t msg_size, const void* msg, void* ctx) {
    (void) msg_size;
    struct uavcan_instance_s* instance = ctx;

    const struct can_rx_frame_s* frame = msg;

    CanardCANFrame canard_frame = convert_can_frame_to_CanardCANFrame(&frame->content);

    chMtxLock(&instance->canard_mtx);
    uint64_t timestamp = micros64();
    canardHandleRxFrame(&instance->canard, &canard_frame, timestamp);
    chMtxUnlock(&instance->canard_mtx);
}

static void stale_transfer_cleanup_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    struct uavcan_instance_s* instance = NULL;

    while (uavcan_iterate_instances(&instance)) {
        chMtxLock(&instance->canard_mtx);
        canardCleanupStaleTransfers(&instance->canard, micros64());
        chMtxUnlock(&instance->canard_mtx);
    }
}

static struct uavcan_instance_s* uavcan_get_instance(uint8_t idx) {
    struct uavcan_instance_s* instance = uavcan_instance_list_head;
    while (instance && idx != 0) {
        idx--;
        instance = instance->next;
    }

    if (idx != 0) {
        return NULL;
    } else {
        return instance;
    }
}

uint8_t uavcan_get_num_instances(void) {
    struct uavcan_instance_s* instance = uavcan_instance_list_head;
    uint8_t count = 0;
    while (instance) {
        count++;
        instance = instance->next;
    }
    return count;
}

static uint8_t uavcan_get_idx(struct uavcan_instance_s* instance_arg) {
    uint8_t idx = 0;
    struct uavcan_instance_s* instance = uavcan_instance_list_head;
    while (instance && instance != instance_arg) {
        idx++;
        instance = instance->next;
    }
    return idx;
}

static void convert_CanardCANFrame_to_can_frame(const CanardCANFrame* canard_frame, struct can_frame_s* ret) {
    ret->IDE = (canard_frame->id & CANARD_CAN_FRAME_EFF) != 0;
    ret->RTR = (canard_frame->id & CANARD_CAN_FRAME_RTR) != 0;
    if (ret->IDE) {
        ret->EID = canard_frame->id & CANARD_CAN_EXT_ID_MASK;
    } else {
        ret->SID = canard_frame->id & CANARD_CAN_STD_ID_MASK;
    }
    ret->DLC = canard_frame->data_len;
    memcpy(ret->data, canard_frame->data, ret->DLC);
}

static CanardCANFrame convert_can_frame_to_CanardCANFrame(const struct can_frame_s* frame) {
    CanardCANFrame ret;
    if (frame->IDE) {
        ret.id = frame->EID | CANARD_CAN_FRAME_EFF;
    } else {
        ret.id = frame->SID;
    }

    if (frame->RTR) {
        ret.id |= CANARD_CAN_FRAME_RTR;
    }

    ret.data_len = frame->DLC;
    memcpy(ret.data, frame->data, ret.data_len);
    return ret;
}

struct uavcan_message_writer_func_args {
    uint8_t uavcan_idx;
    CanardRxTransfer* transfer;
    const struct uavcan_message_descriptor_s* const descriptor;
};

static void uavcan_message_writer_func(size_t msg_size, void* write_buf, void* ctx) {
    (void)msg_size;
    struct uavcan_message_writer_func_args* args = ctx;
    struct uavcan_deserialized_message_s* deserialized_message = write_buf;
    deserialized_message->uavcan_idx = args->uavcan_idx;
    deserialized_message->descriptor = args->descriptor;
    deserialized_message->data_type_id = args->transfer->data_type_id;
    deserialized_message->transfer_id = args->transfer->transfer_id;
    deserialized_message->priority = args->transfer->priority;
    deserialized_message->source_node_id = args->transfer->source_node_id;
    args->descriptor->deserializer_func(args->transfer, deserialized_message->msg);
}

static void uavcan_on_transfer_rx(CanardInstance* canard, CanardRxTransfer* transfer) {
    if (!canard || !transfer) {
        return;
    }

    struct uavcan_instance_s* instance = canardGetUserReference(canard);
    if (!instance) {
        return;
    }

    struct uavcan_rx_list_item_s* rx_list_item = instance->rx_list_head;
    while (rx_list_item) {
        if (rx_list_item->msg_descriptor->transfer_type == transfer->transfer_type && _uavcan_get_message_data_type_id(instance, rx_list_item->msg_descriptor) == transfer->data_type_id) {
            struct uavcan_message_writer_func_args writer_args = { instance->idx, transfer, rx_list_item->msg_descriptor };
            pubsub_publish_message(&rx_list_item->topic, rx_list_item->msg_descriptor->deserialized_size+sizeof(struct uavcan_deserialized_message_s), uavcan_message_writer_func, &writer_args);
        }

        rx_list_item = rx_list_item->next;
    }
}

static bool uavcan_should_accept_transfer(const CanardInstance* canard, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id) {
    (void)source_node_id;
    if (!canard || !out_data_type_signature) {
        return false;
    }

    struct uavcan_instance_s* instance = canardGetUserReference((CanardInstance*)canard);
    if (!instance) {
        return false;
    }

    struct uavcan_rx_list_item_s* rx_list_item = instance->rx_list_head;
    while (rx_list_item) {
        if (transfer_type == rx_list_item->msg_descriptor->transfer_type && data_type_id == _uavcan_get_message_data_type_id(instance, rx_list_item->msg_descriptor)) {
            *out_data_type_signature = rx_list_item->msg_descriptor->data_type_signature;
            return true;
        }

        rx_list_item = rx_list_item->next;
    }

    return false;
}

#define UAVCAN_TRANSFER_ID_MAP_MAX_LEN ((1<<7)-1)

static void uavcan_transfer_id_map_init(struct transfer_id_map_s* map, size_t map_mem_size, void* map_mem) {
    if (!map) {
        return;
    }
    map->entries = map_mem;
    map->size = map_mem_size/sizeof(struct map_entry_s);
    if (map->size > UAVCAN_TRANSFER_ID_MAP_MAX_LEN) {
        map->size = UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
    }
    map->head = UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
}

static uint8_t* uavcan_transfer_id_map_retrieve(struct transfer_id_map_s* map, bool service_not_message, uint16_t data_type_id, uint8_t dest_node_id) {
    if (!map || !map->entries) {
        return 0;
    }

    uint32_t key;
    if (service_not_message) {
        key = (1<<16) | ((data_type_id << 8) & 0xFF00) | ((dest_node_id << 0) & 0x00FF);
    } else {
        key = data_type_id;
    }

    uint16_t count = 0;
    uint16_t entry = map->head;
    uint16_t entry_prev = UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
    uint16_t entry_prev_prev = UAVCAN_TRANSFER_ID_MAP_MAX_LEN;

    while (entry != UAVCAN_TRANSFER_ID_MAP_MAX_LEN && map->entries[entry].key != key) {
        count++;
        entry_prev_prev = entry_prev;
        entry_prev = entry;
        entry = map->entries[entry].next;
    }

    if (entry == UAVCAN_TRANSFER_ID_MAP_MAX_LEN) {
        // Not found. Allocate an entry.
        if (count >= map->size) {
            // list is full - entry_prev is the LRU entry
            entry = entry_prev;
            entry_prev = entry_prev_prev;
        } else {
            // list is not full - allocate next available element
            entry = count;
        }

        // Populate the allocated entry
        map->entries[entry].key = key;
        map->entries[entry].transfer_id = 0;
        map->entries[entry].next = UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
    }

    // Move to front
    if (entry_prev != UAVCAN_TRANSFER_ID_MAP_MAX_LEN) {
        map->entries[entry_prev].next = map->entries[entry].next;
        map->entries[entry].next = map->head;
    }
    map->head = entry;

    return &map->entries[entry].transfer_id;
}

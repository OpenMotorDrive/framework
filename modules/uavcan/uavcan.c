#include <modules/uavcan/uavcan.h>
#include <common/ctor.h>
#include <modules/timing/timing.h>
#include <common/helpers.h>
#include <string.h>
#include <modules/can/can.h>
#include <modules/worker_thread/worker_thread.h>
#include <app_config.h>

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
#define UAVCAN_CANARD_MEMORY_POOL_SIZE 768
#endif

#ifndef UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE
#define UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE 128
#endif

#ifndef UAVCAN_RX_WORKER_THREAD
#error Please define UAVCAN_RX_WORKER_THREAD in framework_conf.h.
#endif

#define WT_RX UAVCAN_RX_WORKER_THREAD

WORKER_THREAD_DECLARE_EXTERN(WT_RX)

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
    struct can_instance_s* can_instance;
    CanardInstance canard;
    void* canard_memory_pool;
    struct transfer_id_map_s transfer_id_map;

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

static CanardCANFrame convert_can_frame_to_CanardCANFrame(const struct can_frame_s* frame);

static void uavcan_transfer_id_map_init(struct transfer_id_map_s* map, size_t map_mem_size, void* map_mem);
static uint8_t* uavcan_transfer_id_map_retrieve(struct transfer_id_map_s* map, bool service_not_message, uint16_t transfer_id, uint8_t dest_node_id);

MEMORYPOOL_DECL(rx_list_pool, sizeof(struct uavcan_rx_list_item_s), chCoreAllocAlignedI);

static void stale_transfer_cleanup_task_func(struct worker_thread_timer_task_s* task);
static struct worker_thread_timer_task_s stale_transfer_cleanup_task;

static struct uavcan_instance_s* uavcan_instance_list_head;

#ifdef MODULE_PARAM_ENABLED
#include <modules/param/param.h>
#ifndef UAVCAN_DEFAULT_NODE_ID
#define UAVCAN_DEFAULT_NODE_ID 0
#endif
PARAM_DEFINE_UINT8_PARAM_STATIC(node_id_param, "uavcan.node_id", UAVCAN_DEFAULT_NODE_ID, 0, 125)
#endif

RUN_ON(UAVCAN_INIT) {
    uavcan_init(0);

    worker_thread_add_timer_task(&WT_RX, &stale_transfer_cleanup_task, stale_transfer_cleanup_task_func, NULL, LL_US2ST(CANARD_RECOMMENDED_STALE_TRANSFER_CLEANUP_INTERVAL_USEC), true);
}

static void uavcan_init(uint8_t can_dev_idx) {
    struct uavcan_instance_s* instance;
    struct can_instance_s* can_instance;
    void* transfer_id_map_working_area;

    if (!(can_instance = can_get_instance(can_dev_idx))) { goto fail; }
    if (!(instance = chCoreAlloc(sizeof(struct uavcan_instance_s)))) { goto fail; }
    memset(instance, 0, sizeof(struct uavcan_instance_s));
    instance->can_instance = can_instance;
    if (!(transfer_id_map_working_area = chCoreAlloc(UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE))) { goto fail; }
    uavcan_transfer_id_map_init(&instance->transfer_id_map, UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE, transfer_id_map_working_area);
    if(!(instance->canard_memory_pool = chCoreAlloc(UAVCAN_CANARD_MEMORY_POOL_SIZE))) { goto fail; }
    canardInit(&instance->canard, instance->canard_memory_pool, UAVCAN_CANARD_MEMORY_POOL_SIZE, uavcan_on_transfer_rx, uavcan_should_accept_transfer, instance);
    struct pubsub_topic_s* can_rx_topic = can_get_rx_topic(instance->can_instance);
    if (!can_rx_topic) { goto fail; }
    worker_thread_add_listener_task(&WT_RX, &instance->rx_listener_task, can_rx_topic, uavcan_can_rx_handler, instance); // TODO configurable thread

    can_set_auto_retransmit_mode(instance->can_instance, false);

    LINKED_LIST_APPEND(struct uavcan_instance_s, uavcan_instance_list_head, instance);

    instance->idx = uavcan_get_idx(instance);

    uint8_t node_id = 0;

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
    {
        const struct shared_app_parameters_s* shared_parameters = shared_get_parameters(&shared_app_descriptor);
        if (shared_parameters && shared_parameters->canbus_local_node_id > 0 && shared_parameters->canbus_local_node_id <= 127) {
            node_id = shared_parameters->canbus_local_node_id;
        }
    }
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
    if (get_boot_msg_valid() && boot_msg.canbus_info.local_node_id > 0 && boot_msg.canbus_info.local_node_id <= 127) {
        node_id = boot_msg.canbus_info.local_node_id;
    }
#endif

#ifdef MODULE_PARAM_ENABLED
    if (node_id_param > 0 && node_id_param <= 127) {
        node_id = node_id_param;
    }
#endif

    _uavcan_set_node_id(instance, node_id);

    return;

fail:
    chSysHalt(NULL);
}

void uavcan_forget_nodeid(uint8_t uavcan_idx) {
    struct uavcan_instance_s* uavcan_instance;
    if (!(uavcan_instance = uavcan_get_instance(uavcan_idx))) { goto fail; }
    CanardInstance* canard_instance= &uavcan_instance->canard;
    canardForgetLocalNodeID(canard_instance);
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

    chSysLock();

    // attempt to find existing item in receive list
    struct uavcan_rx_list_item_s* rx_list_item = instance->rx_list_head;
    while (rx_list_item && rx_list_item->msg_descriptor != msg_descriptor) {
        rx_list_item = rx_list_item->next;
    }

    if (rx_list_item) {
        chSysUnlock();
        return &rx_list_item->topic;
    }

    // create new item in receive list
    rx_list_item = chPoolAllocI(&rx_list_pool);
    if (!rx_list_item) {
        chSysUnlock();
        return NULL;
    }

    // populate it
    rx_list_item->msg_descriptor = msg_descriptor;
    pubsub_init_topic(&rx_list_item->topic, NULL);

    // append it
    LINKED_LIST_APPEND(struct uavcan_rx_list_item_s, instance->rx_list_head, rx_list_item);

    chSysUnlock();

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

    chSysLock();
    uint8_t ret = canardGetLocalNodeID(&instance->canard);
    chSysUnlock();
    return ret;
}

uint8_t uavcan_get_node_id(uint8_t uavcan_idx) {
    return _uavcan_get_node_id(uavcan_get_instance(uavcan_idx));
}

static void _uavcan_set_node_id(struct uavcan_instance_s* instance, uint8_t node_id) {
    if (!instance) {
        return;
    }

    can_set_auto_retransmit_mode(instance->can_instance, node_id != 0);
    chSysLock();
    canardSetLocalNodeID(&instance->canard, node_id);
    chSysUnlock();
}

void uavcan_set_node_id(uint8_t uavcan_idx, uint8_t node_id) {
    return _uavcan_set_node_id(uavcan_get_instance(uavcan_idx), node_id);
}

struct uavcan_transmit_state_s {
    bool failed;
    struct uavcan_instance_s* instance;
    struct can_tx_frame_s* frame_list_head;
    struct can_tx_frame_s* frame_list_tail;
    size_t frame_bit_ofs;
};

/**
 * Bit array copy routine, originally developed by Ben Dyer for Libuavcan. Thanks Ben.
 */

static void __attribute__((optimize("O3"))) copy_bit_array(const uint8_t* src, uint32_t src_offset, uint32_t src_len, uint8_t* dst, uint32_t dst_offset) {
    // Normalizing inputs
    src += src_offset / 8;
    dst += dst_offset / 8;

    src_offset %= 8;
    dst_offset %= 8;

    const size_t last_bit = src_offset + src_len;
    while (last_bit - src_offset)
    {
        const uint8_t src_bit_offset = (uint8_t)(src_offset % 8U);
        const uint8_t dst_bit_offset = (uint8_t)(dst_offset % 8U);

        const uint8_t max_offset = MAX(src_bit_offset, dst_bit_offset);
        const uint32_t copy_bits = MIN(last_bit - src_offset, 8U - max_offset);

        const uint8_t write_mask = (uint8_t)((uint8_t)(0xFF00U >> copy_bits) >> dst_bit_offset);
        const uint8_t src_data = (uint8_t)((src[src_offset / 8U] << src_bit_offset) >> dst_bit_offset);

        dst[dst_offset / 8U] = (uint8_t)((dst[dst_offset / 8U] & ~write_mask) | (src_data & write_mask));

        src_offset += copy_bits;
        dst_offset += copy_bits;
    }
}

static void __attribute__((optimize("O3"))) uavcan_transmit_chunk_handler(uint8_t* chunk, size_t bitlen, void* ctx) {
    struct uavcan_transmit_state_s* tx_state = ctx;

    if (tx_state->failed || bitlen == 0) {
        return;
    }

    if (!tx_state->frame_list_tail) {
        tx_state->frame_list_tail = can_allocate_tx_frame_and_append(tx_state->instance->can_instance, &tx_state->frame_list_head);
        if (!tx_state->frame_list_tail) {
            tx_state->failed = true;
            return;
        }
        memset(tx_state->frame_list_tail->content.data, 0, 8);
    }

    size_t chunk_bit_ofs = 0;

    while (chunk_bit_ofs < bitlen) {
        size_t frame_copy_bits = MIN(bitlen-chunk_bit_ofs, 7*8-tx_state->frame_bit_ofs);
        if (frame_copy_bits == 0) {
            bool make_room_for_crc = tx_state->frame_list_head->next == NULL;
            tx_state->frame_list_tail = can_allocate_tx_frame_and_append(tx_state->instance->can_instance, &tx_state->frame_list_head);
            if (!tx_state->frame_list_tail) {
                tx_state->failed = true;
                return;
            }
            memset(tx_state->frame_list_tail->content.data, 0, 8);
            if (make_room_for_crc) {
                memcpy(tx_state->frame_list_tail->content.data, &tx_state->frame_list_head->content.data[5], 2);
                memmove(&tx_state->frame_list_head->content.data[2], tx_state->frame_list_head->content.data, 5);
                tx_state->frame_bit_ofs = 16;
            } else {
                tx_state->frame_bit_ofs = 0;
            }
            continue;
        }

        copy_bit_array(chunk, chunk_bit_ofs, frame_copy_bits, tx_state->frame_list_tail->content.data, tx_state->frame_bit_ofs);
        chunk_bit_ofs += frame_copy_bits;
        tx_state->frame_bit_ofs += frame_copy_bits;
        tx_state->frame_list_tail->content.DLC = (tx_state->frame_bit_ofs+7)/8 + 1;
    }
}

static bool _uavcan_send(struct uavcan_instance_s* instance, const struct uavcan_message_descriptor_s* const msg_descriptor, uint16_t data_type_id, uint8_t priority, uint8_t transfer_id, uint8_t dest_node_id, void* msg_data) {
    if (!instance || !msg_descriptor || !msg_descriptor->serializer_func || !msg_data) {
        return false;
    }

    if (_uavcan_get_node_id(instance) == 0 && (data_type_id > 0b11 || msg_descriptor->transfer_type != CanardTransferTypeBroadcast)) {
        return false;
    }

    if (msg_descriptor->transfer_type != CanardTransferTypeBroadcast && data_type_id > 0xff) {
        return false;
    }

    if (_uavcan_get_node_id(instance) > 127) {
        return false;
    }

    struct uavcan_transmit_state_s tx_state = {
        false, instance, NULL, NULL, 0
    };

    msg_descriptor->serializer_func(msg_data, uavcan_transmit_chunk_handler, &tx_state);
    if (tx_state.failed || !tx_state.frame_list_head) {
        can_free_tx_frames(instance->can_instance, &tx_state.frame_list_head);
        return false;
    }


    uint32_t can_id = 0;
    can_id |= (uint32_t)(priority&0x1f) << 24;
    if (msg_descriptor->transfer_type == CanardTransferTypeBroadcast) {
        can_id |= (uint32_t)(data_type_id) << 8;
    } else {
        can_id |= data_type_id<<16;
        if (msg_descriptor->transfer_type == CanardTransferTypeRequest) {
            can_id |= 1<<15;
        }
        can_id |= dest_node_id<<8;
        can_id |= 1<<7;
    }

    if (_uavcan_get_node_id(instance) == 0) {
        can_id |= (uint32_t)(crc16_ccitt(tx_state.frame_list_head->content.data, 7, 0xffff) & 0xfffc)<<8;
    }

    can_id |= _uavcan_get_node_id(instance);

    uint8_t toggle = 0;
    struct can_tx_frame_s* frame = tx_state.frame_list_head;
    while (frame != NULL) {
        frame->content.IDE = 1;
        frame->content.RTR = 0;
        frame->content.EID = can_id;
        if (frame == tx_state.frame_list_head) {
            frame->content.data[frame->content.DLC-1] |= 1<<7;
        }
        if (frame == tx_state.frame_list_tail) {
            frame->content.data[frame->content.DLC-1] |= 1<<6;
        }
        frame->content.data[frame->content.DLC-1] |= toggle << 5;
        frame->content.data[frame->content.DLC-1] |= transfer_id&0x1f;

        toggle = toggle?0:1;

        frame = frame->next;
    }

    if (tx_state.frame_list_head != tx_state.frame_list_tail) {
        uint16_t crc16 = crc16_ccitt((void*)&msg_descriptor->data_type_signature, 8, 0xffff);

        frame = tx_state.frame_list_head;
        crc16 = crc16_ccitt(&frame->content.data[2], 5, crc16);
        frame = frame->next;

        while (frame->next != NULL) {
            crc16 = crc16_ccitt(&frame->content.data[0], 7, crc16);
            frame = frame->next;
        }
        crc16 = crc16_ccitt(&frame->content.data[0], frame->content.DLC-1, crc16);
        memcpy(tx_state.frame_list_head->content.data, &crc16, 2);
    }

    can_enqueue_tx_frames(instance->can_instance, &tx_state.frame_list_head, TIME_INFINITE, NULL);

    return true;
}

bool uavcan_broadcast(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* const msg_descriptor, uint8_t priority, void* msg_data) {
    struct uavcan_instance_s* instance = uavcan_get_instance(uavcan_idx);
    if (!instance) {
        return false;
    }

    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    chSysLock();
    uint8_t* transfer_id = uavcan_transfer_id_map_retrieve(&instance->transfer_id_map, false, data_type_id, 0);
    chSysUnlock();
    if(_uavcan_send(instance, msg_descriptor, data_type_id, priority, *transfer_id, 0, msg_data)) {
        (*transfer_id)++;
        return true;
    } else {
        return false;
    }
}

bool uavcan_request(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* const msg_descriptor, uint8_t priority, uint8_t dest_node_id, void* msg_data) {
    struct uavcan_instance_s* instance = uavcan_get_instance(uavcan_idx);
    if (!instance) {
        return false;
    }

    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    chSysLock();
    uint8_t* transfer_id = uavcan_transfer_id_map_retrieve(&instance->transfer_id_map, false, data_type_id, 0);
    chSysUnlock();
    if(_uavcan_send(instance, msg_descriptor, data_type_id, priority, *transfer_id, dest_node_id, msg_data)) {
        (*transfer_id)++;
        return true;
    } else {
        return false;
    }
}

bool uavcan_respond(uint8_t uavcan_idx, const struct uavcan_deserialized_message_s* const req_msg, void* msg_data) {
    struct uavcan_instance_s* instance = uavcan_get_instance(uavcan_idx);
    if (!instance) {
        return false;
    }

    const struct uavcan_message_descriptor_s* msg_descriptor = req_msg->descriptor->resp_descriptor;
    uint8_t priority = req_msg->priority;
    uint8_t transfer_id = req_msg->transfer_id;
    uint8_t dest_node_id = req_msg->source_node_id;
    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    return _uavcan_send(instance, msg_descriptor, data_type_id, priority, transfer_id, dest_node_id, msg_data);
}

static void uavcan_can_rx_handler(size_t msg_size, const void* msg, void* ctx) {
    (void) msg_size;
    struct uavcan_instance_s* instance = ctx;

    const struct can_rx_frame_s* frame = msg;

    CanardCANFrame canard_frame = convert_can_frame_to_CanardCANFrame(&frame->content);

    uint64_t timestamp = micros64();
    canardHandleRxFrame(&instance->canard, &canard_frame, timestamp);
}

static void stale_transfer_cleanup_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    struct uavcan_instance_s* instance = NULL;

    while (uavcan_iterate_instances(&instance)) {
        canardCleanupStaleTransfers(&instance->canard, micros64());
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

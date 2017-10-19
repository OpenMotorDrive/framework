#include <omd_uavcan/omd_uavcan.h>
#include <common/ctor.h>
#include <common/timing.h>
#include <string.h>

#if CH_CFG_USE_MUTEXES_RECURSIVE != TRUE
#error "CH_CFG_USE_MUTEXES_RECURSIVE required"
#endif

#ifndef OMD_UAVCAN_CANARD_MEMORY_POOL_SIZE
#define OMD_UAVCAN_CANARD_MEMORY_POOL_SIZE 1024
#endif

#ifndef OMD_UAVCAN_RX_THREAD_STACK_SIZE
#define OMD_UAVCAN_RX_THREAD_STACK_SIZE 256
#endif

#ifndef OMD_UAVCAN_TX_THREAD_STACK_SIZE
#define OMD_UAVCAN_TX_THREAD_STACK_SIZE 128
#endif

#ifndef OMD_UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE
#define OMD_UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE 128
#endif

#ifndef OMD_UAVCAN_OUTGOING_MESSAGE_BUF_SIZE
#define OMD_UAVCAN_OUTGOING_MESSAGE_BUF_SIZE 512
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

struct omd_uavcan_rx_list_item_s {
    const struct omd_uavcan_message_descriptor_s* msg_descriptor;
    uint16_t data_type_id;
    struct omd_uavcan_rx_list_item_s* next;
};

struct omd_uavcan_instance_s {
    CANDriver* can_dev;
    CanardInstance canard;
    void* canard_memory_pool;
    thread_t* rx_thread;
    thread_t* tx_thread;
    struct transfer_id_map_s transfer_id_map;
    mutex_t canard_mtx;
    mutex_t tx_mtx;
    binary_semaphore_t tx_thread_semaphore;
    void* outgoing_message_buf;

    struct omd_uavcan_rx_list_item_s* rx_list_head;

    struct omd_uavcan_instance_s* next;
};

static THD_FUNCTION(omd_uavcan_rx_thd_func, arg);
static THD_FUNCTION(omd_uavcan_tx_thd_func, arg);

static struct omd_uavcan_instance_s* omd_uavcan_get_instance(uint8_t idx);
static void omd_uavcan_init(CANDriver* can_dev);

static void omd_uavcan_transmit_frames_async(struct omd_uavcan_instance_s* instance);
static void omd_uavcan_transmit_frames_sync(struct omd_uavcan_instance_s* instance);

static bool omd_uavcan_should_accept_transfer(const CanardInstance* canard, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);
static void omd_uavcan_on_transfer_rx(CanardInstance* canard, CanardRxTransfer* transfer);

static CanardCANFrame convert_CANRxFrame_to_CanardCANFrame(const CANRxFrame* chibios_frame);
static CANTxFrame convert_CanardCANFrame_to_CANTxFrame(const CanardCANFrame* canard_frame);

static void omd_uavcan_transfer_id_map_init(struct transfer_id_map_s* map, size_t map_mem_size, void* map_mem);
static uint8_t* omd_uavcan_transfer_id_map_retrieve(struct transfer_id_map_s* map, bool service_not_message, uint16_t transfer_id, uint8_t dest_node_id);

MEMORYPOOL_DECL(tx_thread_pool, THD_WORKING_AREA_SIZE(OMD_UAVCAN_TX_THREAD_STACK_SIZE), chCoreAllocAlignedI);
MEMORYPOOL_DECL(rx_thread_pool, THD_WORKING_AREA_SIZE(OMD_UAVCAN_RX_THREAD_STACK_SIZE), chCoreAllocAlignedI);

static struct omd_uavcan_instance_s* omd_uavcan_instance_list_head;

RUN_ON(OMD_UAVCAN_INIT) {
    omd_uavcan_init(&CAND1);
}

static void omd_uavcan_init(CANDriver* can_dev) {
    struct omd_uavcan_instance_s* instance;
    void* transfer_id_map_working_area;

    if (!(instance = chCoreAllocAligned(sizeof(struct omd_uavcan_instance_s), PORT_WORKING_AREA_ALIGN))) { goto fail; }
    memset(instance, 0, sizeof(struct omd_uavcan_instance_s));
    instance->can_dev = can_dev;
    chMtxObjectInit(&instance->canard_mtx);
    chMtxObjectInit(&instance->tx_mtx);
    chBSemObjectInit(&instance->tx_thread_semaphore, true);
    if (!(instance->outgoing_message_buf = chCoreAllocAligned(OMD_UAVCAN_OUTGOING_MESSAGE_BUF_SIZE, PORT_WORKING_AREA_ALIGN))) { goto fail; }
    if (!(transfer_id_map_working_area = chCoreAllocAligned(OMD_UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE, PORT_WORKING_AREA_ALIGN))) { goto fail; }
    omd_uavcan_transfer_id_map_init(&instance->transfer_id_map, OMD_UAVCAN_TRANSFER_ID_MAP_WORKING_AREA_SIZE, transfer_id_map_working_area);
    if(!(instance->canard_memory_pool = chCoreAllocAligned(OMD_UAVCAN_CANARD_MEMORY_POOL_SIZE, PORT_WORKING_AREA_ALIGN))) { goto fail; }
    canardInit(&instance->canard, instance->canard_memory_pool, OMD_UAVCAN_CANARD_MEMORY_POOL_SIZE, omd_uavcan_on_transfer_rx, omd_uavcan_should_accept_transfer, instance);
    if (!(instance->tx_thread = chThdCreateFromMemoryPool(&tx_thread_pool, "CANTx", HIGHPRIO-2, omd_uavcan_tx_thd_func, instance))) { goto fail; }
    if (!(instance->rx_thread = chThdCreateFromMemoryPool(&rx_thread_pool, "CANRx", HIGHPRIO-2, omd_uavcan_rx_thd_func, instance))) { goto fail; }

    struct omd_uavcan_instance_s** insert_ptr = &omd_uavcan_instance_list_head;
    while(*insert_ptr) {
        insert_ptr = &(*insert_ptr)->next;
    }
    *insert_ptr = instance;
    return;

fail:
    chSysHalt("omd_uavcan");
}

uint8_t omd_uavcan_get_node_id(uint8_t omd_uavcan_idx) {
    struct omd_uavcan_instance_s* instance = omd_uavcan_get_instance(omd_uavcan_idx);
    if (!instance) {
        return 0;
    }

    chMtxLock(&instance->canard_mtx);
    uint8_t ret = canardGetLocalNodeID(&instance->canard);
    chMtxUnlock(&instance->canard_mtx);
    return ret;
}

void omd_uavcan_set_node_id(uint8_t omd_uavcan_idx, uint8_t node_id) {
    struct omd_uavcan_instance_s* instance = omd_uavcan_get_instance(omd_uavcan_idx);
    if (!instance) {
        return;
    }

    chMtxLock(&instance->canard_mtx);
    canardSetLocalNodeID(&instance->canard, node_id);
    chMtxUnlock(&instance->canard_mtx);
}

void omd_uavcan_broadcast(uint8_t omd_uavcan_idx, const struct omd_uavcan_message_descriptor_s* msg_descriptor, uint8_t priority, void* msg_data) {
    struct omd_uavcan_instance_s* instance = omd_uavcan_get_instance(omd_uavcan_idx);
    if (!instance || !instance->outgoing_message_buf || !msg_descriptor || !msg_descriptor->serializer_func || !msg_data) {
        return;
    }

    uint16_t data_type_id = msg_descriptor->default_data_type_id;
    size_t outgoing_message_len = msg_descriptor->serializer_func(msg_data, instance->outgoing_message_buf);

    chMtxLock(&instance->tx_mtx);
    chMtxLock(&instance->canard_mtx);
    uint8_t* transfer_id = omd_uavcan_transfer_id_map_retrieve(&instance->transfer_id_map, false, data_type_id, 0);
    canardBroadcast(&instance->canard, msg_descriptor->data_type_signature, data_type_id, transfer_id, priority, instance->outgoing_message_buf, outgoing_message_len);
    chMtxUnlock(&instance->canard_mtx);
    chMtxUnlock(&instance->tx_mtx);
    omd_uavcan_transmit_frames_async(instance);
}

// void omd_uavcan_request(struct omd_uavcan_instance_s* instance, const struct omd_uavcan_message_descriptor_s* msg_descriptor, uint16_t data_type_id, uint8_t priority, uint8_t dest_node_id, void* msg_data) {
//     if (!instance || !instance->outgoing_message_buf || !msg_descriptor || !msg_descriptor->serializer_func || !msg_data) {
//         return;
//     }
//
//     chMtxLock(&instance->tx_mtx);
//     size_t outgoing_message_len = msg_descriptor->serializer_func(msg_data, instance->outgoing_message_buf);
//     uint8_t* transfer_id = omd_uavcan_transfer_id_map_retrieve(instance->transfer_id_map, true, data_type_id, dest_node_id);
//     chMtxLock(&instance->canard_mtx);
//     canardRequestOrRespond(&instance->canard, dest_node_id, msg_descriptor->data_type_signature, data_type_id, transfer_id, priority, CanardRequest, instance->outgoing_message_buf, outgoing_message_len);
//     chMtxUnlock(&instance->canard_mtx);
//     chMtxUnlock(&instance->tx_mtx);
//     omd_uavcan_transmit_frames_async();
// }

// TODO respond function

static THD_FUNCTION(omd_uavcan_tx_thd_func, arg) {
    (void)arg;
    struct omd_uavcan_instance_s* instance = (struct omd_uavcan_instance_s*)arg;

    if (!instance) {
        chThdExit(0);
    }

    while (true) {
        chBSemWait(&instance->tx_thread_semaphore);
        omd_uavcan_transmit_frames_sync(instance);
    }

    chThdExit(0);
}

static void omd_uavcan_transmit_frames_async(struct omd_uavcan_instance_s* instance) {
    if (!instance) {
        return;
    }

    // TODO: transmit thread should probably inherit calling thread priority
    chBSemSignal(&instance->tx_thread_semaphore);
}

static void omd_uavcan_transmit_frames_sync(struct omd_uavcan_instance_s* instance) {
    if (!instance) {
        return;
    }

    chMtxLock(&instance->tx_mtx);
    const CanardCANFrame* canard_frame;
    while (true) {
        chMtxLock(&instance->canard_mtx);
        canard_frame = canardPeekTxQueue(&instance->canard);
        chMtxUnlock(&instance->canard_mtx);

        if (!canard_frame) {
            break;
        }

        CANTxFrame chibios_frame = convert_CanardCANFrame_to_CANTxFrame(canard_frame);

        if (canTransmitTimeout(instance->can_dev, CAN_ANY_MAILBOX, &chibios_frame, TIME_INFINITE) == MSG_OK) {
            chMtxLock(&instance->canard_mtx);
            canardPopTxQueue(&instance->canard);
            chMtxUnlock(&instance->canard_mtx);
        }
    }
    chMtxUnlock(&instance->tx_mtx);
}

static THD_FUNCTION(omd_uavcan_rx_thd_func, arg) {
    (void)arg;
    struct omd_uavcan_instance_s* instance = (struct omd_uavcan_instance_s*)arg;

    while (true) {
        CANRxFrame chibios_frame;
        if (canReceiveTimeout(instance->can_dev, CAN_ANY_MAILBOX, &chibios_frame, TIME_INFINITE)) {
            uint64_t timestamp = micros64();
            CanardCANFrame canard_frame = convert_CANRxFrame_to_CanardCANFrame(&chibios_frame);

            chMtxLock(&instance->canard_mtx);
            canardHandleRxFrame(&instance->canard, &canard_frame, timestamp);
            chMtxUnlock(&instance->canard_mtx);
        }
    }
}

static struct omd_uavcan_instance_s* omd_uavcan_get_instance(uint8_t idx) {
    struct omd_uavcan_instance_s* instance = omd_uavcan_instance_list_head;
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

static CanardCANFrame convert_CANRxFrame_to_CanardCANFrame(const CANRxFrame* chibios_frame) {
    CanardCANFrame ret;
    if (chibios_frame->IDE) {
        ret.id = chibios_frame->EID | CANARD_CAN_FRAME_EFF;
    } else {
        ret.id = chibios_frame->SID;
    }

    if (chibios_frame->RTR) {
        ret.id |= CANARD_CAN_FRAME_RTR;
    }

    ret.data_len = chibios_frame->DLC;
    memcpy(ret.data, chibios_frame->data8, ret.data_len);
    return ret;
}

static CANTxFrame convert_CanardCANFrame_to_CANTxFrame(const CanardCANFrame* canard_frame) {
    CANTxFrame ret;
    ret.IDE = (canard_frame->id & CANARD_CAN_FRAME_EFF) != 0;
    ret.RTR = (canard_frame->id & CANARD_CAN_FRAME_RTR) != 0;
    if (ret.IDE) {
        ret.EID = canard_frame->id & CANARD_CAN_EXT_ID_MASK;
    } else {
        ret.SID = canard_frame->id & CANARD_CAN_STD_ID_MASK;
    }
    ret.DLC = canard_frame->data_len;
    memcpy(ret.data8, canard_frame->data, ret.DLC);
    return ret;
}

struct omd_uavcan_message_writer_func_args {
    struct omd_uavcan_instance_s* omd_uavcan_instance;
    CanardRxTransfer* transfer;
    omd_uavcan_deserializer_func_ptr deserializer_func;
};

static void omd_uavcan_message_writer_func(size_t msg_size, void* write_buf, void* ctx) {
    (void)msg_size;
    struct omd_uavcan_message_writer_func_args* args = ctx;
    struct omd_uavcan_deserialized_message_s* deserialized_message = write_buf;
    deserialized_message->omd_uavcan_instance = args->omd_uavcan_instance;
    deserialized_message->data_type_id = args->transfer->data_type_id;
    deserialized_message->transfer_id = args->transfer->transfer_id;
    deserialized_message->priority = args->transfer->priority;
    deserialized_message->source_node_id = args->transfer->source_node_id;
    args->deserializer_func(args->transfer, deserialized_message->msg);
}

static void omd_uavcan_on_transfer_rx(CanardInstance* canard, CanardRxTransfer* transfer) {
    if (!canard || !transfer) {
        return;
    }

    struct omd_uavcan_instance_s* instance = canardGetUserReference(canard);

//     struct omd_uavcan_message_subscription_s* message_subscription = instance->message_subscription_list;
//     while (message_subscription) {
//         if (transfer->data_type_id == message_subscription->data_type_id && transfer->transfer_type == message_subscription->transfer_type && message_subscription->deserializer_func) {
//             struct omd_uavcan_message_writer_func_args writer_args = { instance, transfer, message_subscription->deserializer_func };
//             pubsub_publish_message(message_subscription->pubsub_topic, message_subscription->deserialized_size, omd_uavcan_message_writer_func, &writer_args);
//         }
//
//         message_subscription = message_subscription->next;
//     }
}

static bool omd_uavcan_should_accept_transfer(const CanardInstance* canard, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id) {
    (void)source_node_id;
    if (!canard || !out_data_type_signature) {
        return false;
    }

    struct omd_uavcan_instance_s* instance = canardGetUserReference((CanardInstance*)canard);

//     struct omd_uavcan_message_subscription_s* message_subscription = instance->message_subscription_list;
//     while (message_subscription) {
//         if (data_type_id == message_subscription->data_type_id && transfer_type == message_subscription->transfer_type && !memcmp(&message_subscription->data_type_signature, out_data_type_signature, sizeof(uint64_t))) {
//             return true;
//         }
//
//         message_subscription = message_subscription->next;
//     }

    return false;
}

#define OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN ((1<<7)-1)

static void omd_uavcan_transfer_id_map_init(struct transfer_id_map_s* map, size_t map_mem_size, void* map_mem) {
    if (!map) {
        return;
    }
    map->entries = map_mem;
    map->size = map_mem_size/sizeof(struct map_entry_s);
    if (map->size > OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN) {
        map->size = OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
    }
    map->head = OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
}

static uint8_t* omd_uavcan_transfer_id_map_retrieve(struct transfer_id_map_s* map, bool service_not_message, uint16_t data_type_id, uint8_t dest_node_id) {
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
    uint16_t entry_prev = OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
    uint16_t entry_prev_prev = OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN;

    while (entry != OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN && map->entries[entry].key != key) {
        count++;
        entry_prev_prev = entry_prev;
        entry_prev = entry;
        entry = map->entries[entry].next;
    }

    if (entry == OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN) {
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
        map->entries[entry].next = OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN;
    }

    // Move to front
    if (entry_prev != OMD_UAVCAN_TRANSFER_ID_MAP_MAX_LEN) {
        map->entries[entry_prev].next = map->entries[entry].next;
    }
    map->entries[entry].next = map->head;
    map->head = entry;

    return &map->entries[entry].transfer_id;
}

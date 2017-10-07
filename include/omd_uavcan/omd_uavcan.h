#pragma once

#include <canard.h>
#include <ch.h>
#include <hal.h>

typedef void (*deserializer_func_ptr)(CanardRxTransfer* transfer, void* buf);

struct omd_uavcan_message_descriptor_s {
    uint64_t data_type_signature;
    CanardTransferType transfer_type;
    size_t max_deserialized_size;
    deserializer_func_ptr deserializer_func;
};

struct omd_uavcan_listener_s {
    thread_t* listener_thread;
    mailbox_t mailbox;
};

struct omd_uavcan_subscription_s {
    uint8_t message_idx;
    uint16_t data_type_id;
    event_source_t event_source;
    mailbox_t mailbox;
    struct omd_uavcan_subscriber_list_item_s* subscriber_list;
    struct omd_uavcan_subscription_list_item_s* next_item;
};

struct omd_uavcan_instance_s {
    CANDriver* can_dev;
    CanardInstance canard;
    void* canard_memory_pool;
    thread_t* rx_thread;
    thread_t* tx_thread;
    mutex_t canard_mtx;
    mutex_t tx_mtx;
    binary_semaphore_t tx_thread_semaphore;

    struct omd_uavcan_subscription_list_item_s* message_subscription_list;
    memory_heap_t message_heap;
};

void omd_uavcan_init(struct omd_uavcan_instance_s* instance, CANDriver* can_dev, void* message_heap_mem, size_t message_heap_size);
void omd_uavcan_transmit_async(struct omd_uavcan_instance_s* instance);
void omd_uavcan_transmit_sync(struct omd_uavcan_instance_s* instance);

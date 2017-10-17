#pragma once

#include <canard.h>
#include <ch.h>
#include <hal.h>
#include <common/pubsub.h>

typedef void (*omd_uavcan_deserializer_func_ptr)(CanardRxTransfer* transfer, void* buf);
typedef void (*omd_uavcan_serializer_func_ptr)(void* outbuf, void* inbuf);

struct omd_uavcan_deserialized_message_s {
    struct omd_uavcan_instance_s* omd_uavcan_instance;
    uint16_t data_type_id;
    uint8_t transfer_id;
    uint8_t priority;
    uint8_t source_node_id;
    uint8_t msg[];
};

struct omd_uavcan_service_server_s {
    uint64_t data_type_signature;
    uint16_t data_type_id;
    CanardTransferType transfer_type;
    uint8_t priority;
};

struct omd_uavcan_service_client_s {
    uint64_t data_type_signature;
    uint16_t data_type_id;
    CanardTransferType transfer_type;
    uint8_t priority;
};

struct omd_uavcan_message_broadcaster_s {
    uint64_t data_type_signature;
    uint16_t data_type_id;
    CanardTransferType transfer_type;
    uint8_t priority;
    uint8_t transfer_id;
    omd_uavcan_serializer_func_ptr serializer_func;
};

struct omd_uavcan_message_subscription_s {
    uint64_t data_type_signature;
    uint16_t data_type_id;
    CanardTransferType transfer_type;
    size_t deserialized_size;
    omd_uavcan_deserializer_func_ptr deserializer_func;
    struct pubsub_topic_s* pubsub_topic;
    struct omd_uavcan_message_subscription_s* next;
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

    struct omd_uavcan_message_subscription_s* message_subscription_list;
};

void omd_uavcan_init(struct omd_uavcan_instance_s* instance, CANDriver* can_dev);
void omd_uavcan_add_sub(struct omd_uavcan_instance_s* instance, struct omd_uavcan_message_subscription_s* new_sub);
void omd_uavcan_transmit_async(struct omd_uavcan_instance_s* instance);
void omd_uavcan_transmit_sync(struct omd_uavcan_instance_s* instance);

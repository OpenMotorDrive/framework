#pragma once

#include <canard.h>
#include <ch.h>
#include <hal.h>
#include <pubsub/pubsub.h>

typedef uint32_t (*uavcan_serializer_func_ptr_t)(void* msg_struct, void* buffer);
typedef uint32_t (*uavcan_deserializer_func_ptr_t)(CanardRxTransfer* transfer, void* msg_struct);

struct uavcan_message_descriptor_s {
    uint64_t data_type_signature;
    uint16_t default_data_type_id;
    CanardTransferType transfer_type;
    size_t deserialized_size;
    size_t max_serialized_size;
    uavcan_serializer_func_ptr_t serializer_func;
    uavcan_deserializer_func_ptr_t deserializer_func;
};

struct uavcan_deserialized_message_s {
    uint8_t uavcan_idx;
    uint16_t data_type_id;
    uint8_t transfer_id;
    uint8_t priority;
    uint8_t source_node_id;
    uint8_t msg[] __attribute__((aligned));
};

struct uavcan_instance_s;

uint8_t uavcan_get_num_instances(void);

uint8_t uavcan_get_node_id(uint8_t uavcan_idx);
void uavcan_set_node_id(uint8_t uavcan_idx, uint8_t node_id);

uint16_t uavcan_get_message_data_type_id(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor);

struct pubsub_topic_s* uavcan_get_message_topic(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor);

void uavcan_broadcast(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor, uint8_t priority, void* msg_data);
void uavcan_request(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor, uint8_t priority, uint8_t dest_node_id, void* msg_data);
void uavcan_respond(uint8_t uavcan_idx, const struct uavcan_message_descriptor_s* msg_descriptor, uint8_t priority, uint8_t transfer_id, uint8_t dest_node_id, void* msg_data);

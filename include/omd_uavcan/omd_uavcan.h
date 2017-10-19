#pragma once

#include <canard.h>
#include <ch.h>
#include <hal.h>
#include <common/pubsub.h>

typedef uint32_t (*omd_uavcan_deserializer_func_ptr)(CanardRxTransfer* transfer, void* output_buf);
typedef uint32_t (*omd_uavcan_serializer_func_ptr)(void* input_buf, void* output_buf);

struct omd_uavcan_message_descriptor_s {
    uint64_t data_type_signature;
    uint16_t default_data_type_id;
    CanardTransferType transfer_type;
    size_t deserialized_size;
    size_t max_serialized_size;
    omd_uavcan_serializer_func_ptr serializer_func;
    omd_uavcan_deserializer_func_ptr deserializer_func;
};

struct omd_uavcan_deserialized_message_s {
    struct omd_uavcan_instance_s* omd_uavcan_instance;
    uint16_t data_type_id;
    uint8_t transfer_id;
    uint8_t priority;
    uint8_t source_node_id;
    uint8_t msg[];
};

struct omd_uavcan_instance_s;

void omd_uavcan_broadcast(uint8_t omd_uavcan_idx, const struct omd_uavcan_message_descriptor_s* msg_descriptor, uint8_t priority, void* msg_data);
uint8_t omd_uavcan_get_node_id(uint8_t omd_uavcan_idx);
void omd_uavcan_set_node_id(uint8_t omd_uavcan_idx, uint8_t node_id);

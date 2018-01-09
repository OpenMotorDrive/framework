#pragma once

#include <uavcan.protocol.NodeStatus.h>

const struct uavcan_protocol_NodeStatus_s* uavcan_nodestatus_publisher_get_nodestatus_message(void);
void set_node_health(uint8_t health);
void set_node_mode(uint8_t mode);
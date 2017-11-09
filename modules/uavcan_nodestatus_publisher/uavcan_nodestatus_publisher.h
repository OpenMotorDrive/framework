#pragma once

#include <uavcan.protocol.NodeStatus.h>

const struct uavcan_protocol_NodeStatus_s* uavcan_nodestatus_publisher_get_nodestatus_message(void);

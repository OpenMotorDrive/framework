#pragma once

#include <uavcan.protocol.debug.LogMessage.h>

void uavcan_send_debug_msg(uint8_t debug_level, char* source, const char *fmt, ...);

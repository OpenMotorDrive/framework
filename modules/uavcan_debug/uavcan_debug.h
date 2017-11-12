#pragma once

#include <uavcan.protocol.debug.LogMessage.h>
#include <uavcan.protocol.debug.KeyValue.h>

void uavcan_send_debug_msg(uint8_t debug_level, char* source, const char *fmt, ...);
void uavcan_send_debug_keyvalue(char* key, float value);

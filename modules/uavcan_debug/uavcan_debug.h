#pragma once

#include <uavcan.protocol.debug.LogMessage.h>
#include <uavcan.protocol.debug.KeyValue.h>

#define LOG_LEVEL_DEBUG UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_DEBUG
#define LOG_LEVEL_INFO UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_INFO
#define LOG_LEVEL_WARNING UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_WARNING
#define LOG_LEVEL_ERROR UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_ERROR

void uavcan_send_debug_msg(uint8_t debug_level, char* source, const char *fmt, ...);
void uavcan_send_debug_keyvalue(char* key, float value);

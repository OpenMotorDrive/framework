#pragma once

#include <uavcan.protocol.debug.LogLevel.h>

#define LOG_LEVEL_DEBUG UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_DEBUG
#define LOG_LEVEL_INFO UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_INFO
#define LOG_LEVEL_WARNING UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_WARNING
#define LOG_LEVEL_ERROR UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_ERROR

void uavcan_logmessage_printf(uint8_t uavcan_idx, uint8_t log_level, const char* source, const char* format, ... );

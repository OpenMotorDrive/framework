#include <modules/uavcan/uavcan.h>
#include "uavcan_logmessage_printf.h"
#include <uavcan.protocol.debug.LogMessage.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void uavcan_logmessage_printf(uint8_t uavcan_idx, uint8_t log_level, const char* source, const char* format, ... ) {
    va_list args;
    va_start(args, format);

    struct uavcan_protocol_debug_LogMessage_s msg;

    msg.level.value = log_level;

    msg.source_len = strnlen(source, sizeof(msg.source));
    memcpy(msg.source, source, msg.source_len);

    msg.text_len = vsnprintf((char*)msg.text, sizeof(msg.text), format, args);

    uavcan_broadcast(uavcan_idx, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &msg);

    va_end(args);
}

#include "uavcan_debug.h"
#include <uavcan/uavcan.h>
#include <stdio.h>
#include <stdarg.h>

void uavcan_send_debug_msg(uint8_t debug_level, char* source, const char *fmt, ...)
{
    va_list arg;
    int done;
    struct uavcan_protocol_debug_LogMessage_s log_msg;

    va_start (arg, fmt);
    log_msg.text_len =  vsprintf(log_msg.text, fmt, arg);
    va_end (arg);

    memcpy(log_msg.source, source, sizeof(source));
    log_msg.source_len = sizeof(source);
    log_msg.level.value = debug_level;
    uavcan_broadcast(0, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_HIGH, &log_msg);
}

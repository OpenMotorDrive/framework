#include "uavcan_debug.h"
#include <uavcan/uavcan.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <chprintf.h>

void uavcan_send_debug_msg(uint8_t debug_level, char* source, const char *fmt, ...)
{
    va_list arg;
    struct uavcan_protocol_debug_LogMessage_s log_msg;

    va_start (arg, fmt);
    log_msg.text_len =  chsnprintf(log_msg.text, sizeof(log_msg.text),fmt, arg);
    va_end (arg);

    log_msg.source_len = strlen(source);

    memcpy(log_msg.source, source, log_msg.source_len);
    log_msg.level.value = debug_level;
    uavcan_broadcast(0, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_HIGH, &log_msg);
}

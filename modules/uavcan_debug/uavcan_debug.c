#include "uavcan_debug.h"
#include <modules/uavcan/uavcan.h>
#include <common/helpers.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <chprintf.h>
#include <memstreams.h>

void uavcan_send_debug_msg(uint8_t debug_level, char* source, const char *fmt, ...)
{
    struct uavcan_protocol_debug_LogMessage_s log_msg;

    va_list ap;
    MemoryStream ms;
    BaseSequentialStream *chp;

    /* Memory stream object to be used as a string writer, reserving one
     byte for the final zero.*/
    msObjectInit(&ms, (uint8_t *)log_msg.text, sizeof(log_msg.text), 0);

    /* Performing the print operation using the common code.*/
    chp = (BaseSequentialStream *)(void *)&ms;
    va_start(ap, fmt);
    log_msg.text_len = MIN((size_t)chvprintf(chp, fmt, ap), sizeof(log_msg.text));
    va_end(ap);

    log_msg.source_len = strnlen(source, sizeof(log_msg.source));
    memcpy(log_msg.source, source, log_msg.source_len);

    log_msg.level.value = debug_level;

    uavcan_broadcast(0, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &log_msg);
}

void uavcan_send_debug_keyvalue(char* key, float value)
{
    struct uavcan_protocol_debug_KeyValue_s log_kv;
    log_kv.value = value;

    log_kv.key_len = strnlen(key, sizeof(log_kv.key));
    memcpy(log_kv.key, key, log_kv.key_len);

    uavcan_broadcast(0, &uavcan_protocol_debug_KeyValue_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &log_kv);
}

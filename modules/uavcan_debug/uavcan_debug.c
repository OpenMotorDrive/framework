#include "uavcan_debug.h"
#include <modules/uavcan/uavcan.h>
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
    log_msg.text_len = chvprintf(chp, fmt, ap);
    va_end(ap);

    /* Terminate with a zero, unless size==0.*/
    if (ms.eos < sizeof(log_msg.text))
      log_msg.text[ms.eos] = 0;


    log_msg.source_len = strlen(source);

    memcpy(log_msg.source, source, log_msg.source_len);
    log_msg.level.value = debug_level;
    uavcan_broadcast(0, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_HIGH, &log_msg);
}

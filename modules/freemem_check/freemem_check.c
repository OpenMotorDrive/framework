#include <common/ctor.h>
#include <ch.h>

#ifdef MODULE_UAVCAN_DEBUG_ENABLED
#include <modules/uavcan_debug/uavcan_debug.h>
#endif

#ifndef REQUIRED_RAM_MARGIN_AFTER_INIT
#error Please define REQUIRED_RAM_MARGIN_AFTER_INIT (bytes) in framework_conf.h.
#endif

uint8_t _module_freemem_init_phase = 1;

RUN_AFTER(INIT_END) {
    if (chCoreGetStatusX() < REQUIRED_RAM_MARGIN_AFTER_INIT) {
        chSysHalt(NULL);
    }

#ifdef MODULE_UAVCAN_DEBUG_ENABLED
    uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_INFO, "", "freemem %u", chCoreGetStatusX());
#endif

    _module_freemem_init_phase = 0;
}

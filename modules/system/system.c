#include "system.h"

#include <ch.h>

static restart_allowed_func_ptr_t restart_allowed_cb;

void system_set_restart_allowed_cb(restart_allowed_func_ptr_t cb) {
    restart_allowed_cb = cb;
}

bool system_get_restart_allowed(void) {
    return !restart_allowed_cb || restart_allowed_cb();
}

void system_restart(void) {
    NVIC_SystemReset();
}

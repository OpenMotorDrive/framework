#pragma once

#include <stdbool.h>

typedef bool (*restart_allowed_func_ptr_t)(void);

void uavcan_restart_set_restart_allowed_cb(restart_allowed_func_ptr_t cb);

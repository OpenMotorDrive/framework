#pragma once

#include <stdbool.h>

typedef bool (*restart_allowed_func_ptr_t)(void);

void system_set_restart_allowed_cb(restart_allowed_func_ptr_t cb);
bool system_get_restart_allowed(void);
void system_restart(void);

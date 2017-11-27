#pragma once

#include <ch.h>

#define FAULT_DEFINE_STATIC(HANDLE_NAME, LEVEL, DESCRIPTION) \
static struct fault_s HANDLE_NAME; \
RUN_AFTER(CH_SYS_INIT) { \
    fault_register(&HANDLE_NAME, LEVEL, DESCRIPTION); \
}

enum fault_level_t {
    FAULT_LEVEL_OK,
    FAULT_LEVEL_WARNING,
    FAULT_LEVEL_ERROR,
    FAULT_LEVEL_CRITICAL
};

struct fault_s {
    const char* description;
    enum fault_level_t level;
    systime_t raised_time;
    systime_t timeout;
    struct fault_s* next;
};

void fault_register(struct fault_s* fault, enum fault_level_t level, const char* description);

void fault_raise(struct fault_s* fault, systime_t timeout);
void fault_clear(struct fault_s* fault);

enum fault_level_t fault_get_level(struct fault_s* fault);

struct fault_s* fault_get_worst_fault(void);

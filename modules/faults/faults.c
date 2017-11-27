#include "faults.h"
#include <common/helpers.h>

#ifdef MODULE_PUBSUB_ENABLED
#include <modules/pubsub/pubsub.h>
struct pubsub_topic_s fault_raised_topic;
#endif

static struct fault_s* faults_head;

void fault_register(struct fault_s* fault, enum fault_level_t level, const char* description) {
    fault->description = description;
    fault->level = level;
    fault->timeout = TIME_IMMEDIATE;
    fault->next = NULL;

    chSysLock();
    LINKED_LIST_APPEND(struct fault_s, faults_head, fault);
    chSysUnlock();
}

void fault_raise(struct fault_s* fault, systime_t timeout) {
    if (!fault) {
        return;
    }
    
#ifdef MODULE_PUBSUB_ENABLED
    bool edge = (fault_get_level(fault) == FAULT_LEVEL_OK);
#endif
    
    fault->raised_time = chVTGetSystemTimeX();
    fault->timeout = timeout;
    
#ifdef MODULE_PUBSUB_ENABLED
    if (edge) {
        pubsub_publish_message(&fault_raised_topic, sizeof(struct fault_s*), pubsub_copy_writer_func, &fault);
    }
#endif
}

void fault_clear(struct fault_s* fault) {
    if (!fault) {
        return;
    }
    
    fault->timeout = TIME_IMMEDIATE;
}

enum fault_level_t fault_get_level(struct fault_s* fault) {
    if (!fault) {
        return FAULT_LEVEL_OK;
    }

    systime_t systime_now = chVTGetSystemTimeX();
    if (fault->timeout != TIME_INFINITE && (fault->timeout == TIME_IMMEDIATE || systime_now - fault->raised_time > fault->timeout)) {
        fault->timeout = TIME_IMMEDIATE;
        return FAULT_LEVEL_OK;
    }

    return fault->level;
}

struct fault_s* fault_get_worst_fault(void) {
    struct fault_s* ret = NULL;
    
    for (struct fault_s* fault = faults_head; fault != NULL; fault = fault->next) {
        if (fault_get_level(fault) > fault_get_level(ret)) {
            ret = fault;
        }
    }
    
    return ret;
}

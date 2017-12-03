#include "can_tx_queue.h"
#include "can_helpers.h"
#include "string.h"
#include <common/helpers.h>

#include <ch.h>

#if CH_DBG_ENABLE_CHECKS
static bool can_tx_queue_frame_exists_in_queue(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame);
#endif

bool can_tx_queue_init(struct can_tx_queue_s* instance) {
    instance->head = NULL;
    return true;
}

void can_tx_queue_push_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* push_frame) {
    chDbgCheckClassI();
    
#if CH_DBG_ENABLE_CHECKS
    chDbgCheck(!can_tx_queue_frame_exists_in_queue(instance, push_frame));
#endif

    can_frame_priority_t push_frame_prio = can_get_tx_frame_priority_X(push_frame);

    struct can_tx_frame_s** insert_ptr = &instance->head;
    while(*insert_ptr != NULL && push_frame_prio <= can_get_tx_frame_priority_X(*insert_ptr)) {
        insert_ptr = &(*insert_ptr)->next;
    }

    push_frame->next = *insert_ptr;
    *insert_ptr = push_frame;
}

void can_tx_queue_push(struct can_tx_queue_s* instance, struct can_tx_frame_s* push_frame) {
    chSysLock();
    can_tx_queue_push_I(instance, push_frame);
    chSysUnlock();
}

void can_tx_queue_push_ahead_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* push_frame) {
    chDbgCheckClassI();
    
#if CH_DBG_ENABLE_CHECKS
    chDbgCheck(!can_tx_queue_frame_exists_in_queue(instance, push_frame));
#endif

    struct can_tx_frame_s** insert_ptr = &instance->head;

    can_frame_priority_t push_frame_prio = can_get_tx_frame_priority_X(push_frame);
    while(*insert_ptr != NULL && push_frame_prio < can_get_tx_frame_priority_X(*insert_ptr)) {
        insert_ptr = &(*insert_ptr)->next;
    }

    push_frame->next = *insert_ptr;
    *insert_ptr = push_frame;
}

void can_tx_queue_push_ahead(struct can_tx_queue_s* instance, struct can_tx_frame_s* push_frame) {
    chSysLock();
    can_tx_queue_push_ahead_I(instance, push_frame);
    chSysUnlock();
}

bool can_tx_queue_iterate_I(struct can_tx_queue_s* instance, struct can_tx_frame_s** frame_ptr) {
    chDbgCheckClassI();
    chDbgCheck(frame_ptr != NULL);

    if (frame_ptr == NULL) {
        return false;
    }

    if (*frame_ptr == NULL) {
        *frame_ptr = instance->head;
    } else {
        *frame_ptr = (*frame_ptr)->next;
    }

    return *frame_ptr != NULL;
}

void can_tx_queue_remove_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame) {
    chDbgCheckClassI();
#if CH_DBG_ENABLE_CHECKS
    chDbgCheck(can_tx_queue_frame_exists_in_queue(instance, frame));
#endif

    LINKED_LIST_REMOVE(struct can_tx_frame_s, instance->head, frame);
}

struct can_tx_frame_s* can_tx_queue_peek_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();
    
    return instance->head;
}

struct can_tx_frame_s* can_tx_queue_peek(struct can_tx_queue_s* instance) {
    chSysLock();
    struct can_tx_frame_s* ret = can_tx_queue_peek_I(instance);
    chSysUnlock();
    return ret;
}

void can_tx_queue_pop_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();

    if (instance->head) {
        instance->head = instance->head->next;
    }
}

void can_tx_queue_pop(struct can_tx_queue_s* instance) {
    chSysLock();
    can_tx_queue_pop_I(instance);
    chSysUnlock();
}

struct can_tx_frame_s* can_tx_queue_pop_expired_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();
    
    struct can_tx_frame_s* ret = NULL;
    struct can_tx_frame_s** expired_ptr = &instance->head;
    while (*expired_ptr && !can_tx_frame_expired_X(*expired_ptr)) {
        expired_ptr = &(*expired_ptr)->next;
    }
    
    if (*expired_ptr) {
        ret = *expired_ptr;
        *expired_ptr = (*expired_ptr)->next;
    }
    
    return ret;
}

struct can_tx_frame_s* can_tx_queue_pop_expired(struct can_tx_queue_s* instance) {
    struct can_tx_frame_s* ret;
    chSysLock();
    ret = can_tx_queue_pop_expired_I(instance);
    chSysUnlock();
    return ret;
}

#if CH_DBG_ENABLE_CHECKS
static bool can_tx_queue_frame_exists_in_queue(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame) {
    struct can_tx_frame_s* frame = NULL;
    while (can_tx_queue_iterate_I(instance, &frame)) {
        if (check_frame == frame) {
            return true;
        }
    }
    return false;
}
#endif

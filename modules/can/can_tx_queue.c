#include "can_tx_queue.h"
#include "can_helpers.h"
#include "string.h"
#include <common/helpers.h>

#include <ch.h>

#if CH_DBG_ENABLE_CHECKS
static bool can_tx_queue_frame_exists_in_stage(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame);
static bool can_tx_queue_frame_exists_in_queue(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame);
static bool can_tx_queue_frame_exists(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame);
#endif

bool can_tx_queue_init(struct can_tx_queue_s* instance, size_t queue_max_len) {
    chPoolObjectInit(&instance->frame_pool, sizeof(struct can_tx_frame_s), NULL);
    void* queue_mem = chCoreAllocAligned(queue_max_len*sizeof(struct can_tx_frame_s), PORT_WORKING_AREA_ALIGN);
    if (!queue_mem) {
        return false;
    }
    chPoolLoadArray(&instance->frame_pool, queue_mem, queue_max_len);
    instance->head = NULL;
    instance->stage_head = NULL;
    return true;
}

struct can_tx_frame_s* can_tx_queue_allocate_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();

    return chPoolAllocI(&instance->frame_pool);
}

void can_tx_queue_free_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* free_frame) {
    chDbgCheckClassI();

    chPoolFreeI(&instance->frame_pool, free_frame);
}

void can_tx_queue_free(struct can_tx_queue_s* instance, struct can_tx_frame_s* free_frame) {
    chSysLock();
    can_tx_queue_free_I(instance, free_frame);
    chSysUnlock();
}

void can_tx_queue_stage_push_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* push_frame) {
    chDbgCheckClassI();
    
#if CH_DBG_ENABLE_CHECKS
    chDbgCheck(!can_tx_queue_frame_exists(instance, push_frame));
#endif

    LINKED_LIST_APPEND(struct can_tx_frame_s, instance->stage_head, push_frame);
}

void can_tx_queue_push_ahead_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* push_frame) {
    chDbgCheckClassI();
    
#if CH_DBG_ENABLE_CHECKS
    chDbgCheck(!can_tx_queue_frame_exists(instance, push_frame));
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

bool can_tx_stage_iterate_I(struct can_tx_queue_s* instance, struct can_tx_frame_s** frame_ptr) {
    chDbgCheckClassI();
    chDbgCheck(frame_ptr != NULL);

    if (frame_ptr == NULL) {
        return false;
    }

    if (*frame_ptr == NULL) {
        *frame_ptr = instance->stage_head;
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

struct can_tx_frame_s* can_tx_queue_pop_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();

    struct can_tx_frame_s* ret_frame = instance->head;
    if (instance->head) {
        instance->head = instance->head->next;
    }

    return ret_frame;
}

struct can_tx_frame_s* can_tx_queue_pop(struct can_tx_queue_s* instance) {
    chSysLock();
    struct can_tx_frame_s* ret_frame = can_tx_queue_pop_I(instance);
    chSysUnlock();
    return ret_frame;
}

struct can_tx_frame_s* can_tx_queue_peek_I(struct can_tx_queue_s* instance) {
    return instance->head;
}

struct can_tx_frame_s* can_tx_queue_peek(struct can_tx_queue_s* instance) {
    chSysLock();
    struct can_tx_frame_s* ret = can_tx_queue_peek_I(instance);
    chSysUnlock();
    return ret;
}

void can_tx_queue_commit_staged_pushes_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();


    while (instance->stage_head) {
        struct can_tx_frame_s* push_frame = instance->stage_head;
        instance->stage_head = instance->stage_head->next;

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
}

void can_tx_queue_free_staged_pushes_I(struct can_tx_queue_s* instance) {
    chDbgCheckClassI();

    while (instance->stage_head) {
        struct can_tx_frame_s* free_frame = instance->stage_head;
        instance->stage_head = instance->stage_head->next;

        chPoolFreeI(&instance->frame_pool, free_frame);
    }
}

#if CH_DBG_ENABLE_CHECKS
static bool can_tx_queue_frame_exists_in_stage(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame) {
    struct can_tx_frame_s* frame = NULL;
    while (can_tx_stage_iterate_I(instance, &frame)) {
        if (check_frame == frame) {
            return true;
        }
    }
    return false;
}

static bool can_tx_queue_frame_exists_in_queue(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame) {
    struct can_tx_frame_s* frame = NULL;
    while (can_tx_queue_iterate_I(instance, &frame)) {
        if (check_frame == frame) {
            return true;
        }
    }
    return false;
}

static bool can_tx_queue_frame_exists(struct can_tx_queue_s* instance, struct can_tx_frame_s* check_frame) {
    return can_tx_queue_frame_exists_in_stage(instance,check_frame) || can_tx_queue_frame_exists_in_queue(instance,check_frame);
}
#endif

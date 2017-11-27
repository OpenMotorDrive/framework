#include "worker_thread.h"

#include <common/helpers.h>

static THD_FUNCTION(worker_thread_func, arg);
static void worker_thread_insert_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task);

void worker_thread_init(struct worker_thread_s* worker_thread, const char* name, size_t stack_size, tprio_t prio) {
    if (!worker_thread) {
        return;
    }
    void* working_area = chCoreAllocAligned(THD_WORKING_AREA_SIZE(stack_size), PORT_WORKING_AREA_ALIGN);
    if (!working_area) {
        return;
    }
    worker_thread->thread = chThdCreateStatic(working_area, THD_WORKING_AREA_SIZE(stack_size), prio, worker_thread_func, worker_thread);
    worker_thread->name = name;
    worker_thread->next_timer_task = NULL;
#ifdef MODULE_PUBSUB_ENABLED
    worker_thread->listener_task_list_head = NULL;
#endif
    worker_thread->interrupt_task_list_head = NULL;
    worker_thread->waiting = false;
    chMtxObjectInit(&worker_thread->mtx);
}

static void wake_worker_thread(struct worker_thread_s* worker_thread) {
    chSysLock();
    if (worker_thread->waiting) {
        chSchWakeupS(worker_thread->thread, MSG_TIMEOUT);
        //set waiting to false so that we don't try waking twice
        worker_thread->waiting = false;
    }
    chSysUnlock();
}

void worker_thread_add_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, timer_task_handler_func_ptr task_func, void* ctx, systime_t period_ticks, bool auto_repeat) {
    task->task_func = task_func;
    task->ctx = ctx;
    task->period_ticks = period_ticks;
    task->auto_repeat = auto_repeat;
    task->last_run_time_ticks = chVTGetSystemTimeX();

    chMtxLock(&worker_thread->mtx);

    worker_thread_insert_timer_task(worker_thread, task);

    // Wake worker thread to process tasks
    wake_worker_thread(worker_thread);

    chMtxUnlock(&worker_thread->mtx);
}

void worker_thread_remove_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    chMtxLock(&worker_thread->mtx);

    struct worker_thread_timer_task_s** remove_ptr = &worker_thread->next_timer_task;
    while (*remove_ptr && *remove_ptr != task) {
        remove_ptr = &(*remove_ptr)->next;
    }

    if (*remove_ptr) {
        *remove_ptr = task->next;
    }

    chMtxUnlock(&worker_thread->mtx);
}

void* worker_thread_task_get_user_context(struct worker_thread_timer_task_s* task) {
    if (!task) {
        return NULL;
    }

    return task->ctx;
}

#ifdef MODULE_PUBSUB_ENABLED
void worker_thread_add_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task, struct pubsub_listener_s* listener) {
    task->listener = listener;

    chMtxLock(&worker_thread->mtx);

    LINKED_LIST_APPEND(struct worker_thread_listener_task_s, worker_thread->listener_task_list_head, task);

    // Wake worker thread to process tasks
    wake_worker_thread(worker_thread);

    chMtxUnlock(&worker_thread->mtx);
}

void worker_thread_remove_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task) {
    chMtxLock(&worker_thread->mtx);

    pubsub_listener_set_waiting_thread_reference_S(task->listener, NULL);

    struct worker_thread_listener_task_s** remove_ptr = &worker_thread->listener_task_list_head;
    while (*remove_ptr && *remove_ptr != task) {
        remove_ptr = &(*remove_ptr)->next;
    }

    if (*remove_ptr) {
        *remove_ptr = task->next;
    }

    chMtxUnlock(&worker_thread->mtx);
}
#endif

void worker_thread_add_interrupt_task(struct worker_thread_s* worker_thread, struct worker_thread_interrupt_task_s* task, interrupt_task_handler_func_ptr task_func, void* ctx) {
    task->triggered = false;
    task->task_func = task_func;
    task->ctx = ctx;
    task->worker_thread = worker_thread;

    chMtxLock(&worker_thread->mtx);
    LINKED_LIST_APPEND(struct worker_thread_interrupt_task_s, worker_thread->interrupt_task_list_head, task);
    chMtxUnlock(&worker_thread->mtx);
}

void worker_thread_remove_interrupt_task(struct worker_thread_s* worker_thread, struct worker_thread_interrupt_task_s* task) {
    chMtxLock(&worker_thread->mtx);
    struct worker_thread_interrupt_task_s** remove_ptr = &worker_thread->interrupt_task_list_head;
    while (*remove_ptr && *remove_ptr != task) {
        remove_ptr = &(*remove_ptr)->next;
    }
    chMtxUnlock(&worker_thread->mtx);
}

void worker_thread_trigger_interrupt_task_I(struct worker_thread_interrupt_task_s* task) {
    task->triggered = true;

    if (task->worker_thread->waiting) {
        task->worker_thread->thread->u.rdymsg = MSG_TIMEOUT;
        chSchReadyI(task->worker_thread->thread);
        //set waiting to false so that we don't try waking twice
        task->worker_thread->waiting = false;
    }
}

static bool worker_thread_get_interrupt_task_triggered_S(struct worker_thread_s* worker_thread) {
    struct worker_thread_interrupt_task_s* interrupt_task = worker_thread->interrupt_task_list_head;
    while (interrupt_task) {
        if (interrupt_task->triggered) {
            return true;
        }
        chDbgCheck(interrupt_task->next != interrupt_task);
        interrupt_task = interrupt_task->next;
    }
    return false;
}

static void worker_thread_insert_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    systime_t task_run_time = task->last_run_time_ticks + task->period_ticks;
    struct worker_thread_timer_task_s** insert_ptr = &worker_thread->next_timer_task;
    while (*insert_ptr && task_run_time - (*insert_ptr)->last_run_time_ticks >= (*insert_ptr)->period_ticks) {
        insert_ptr = &(*insert_ptr)->next;
    }
    task->next = *insert_ptr;
    *insert_ptr = task;
}

#ifdef MODULE_PUBSUB_ENABLED
static void worker_thread_set_listener_thread_references_S(struct worker_thread_s* worker_thread, thread_reference_t* trpp) {
    struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
    while (listener_task) {
        pubsub_listener_set_waiting_thread_reference_S(listener_task->listener, trpp);
        listener_task = listener_task->next;
    }
}

static bool worker_thread_get_listener_task_waiting_S(struct worker_thread_s* worker_thread) {
    struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
    while (listener_task) {
        if (pubsub_listener_has_message(listener_task->listener)) {
            return true;
        }
        chDbgCheck(listener_task->next != listener_task);
        listener_task = listener_task->next;
    }
    return false;
}
#endif

static systime_t worker_thread_get_ticks_to_next_timer_task(struct worker_thread_s* worker_thread, systime_t tnow_ticks) {
    struct worker_thread_timer_task_s* next_timer_task = worker_thread->next_timer_task;
    if (next_timer_task) {
        systime_t elapsed = tnow_ticks - next_timer_task->last_run_time_ticks;
        if (elapsed >= next_timer_task->period_ticks) {
            return TIME_IMMEDIATE;
        } else {
            return next_timer_task->period_ticks - elapsed;
        }
    } else {
        return TIME_INFINITE;
    }
}

static THD_FUNCTION(worker_thread_func, arg) {
    struct worker_thread_s* worker_thread = arg;
    if (worker_thread->name) {
        chRegSetThreadName(worker_thread->name);
    }

    while (true) {
        // Check for triggered interrupt tasks, handle all
        {
            chMtxLock(&worker_thread->mtx);
            struct worker_thread_interrupt_task_s* interrupt_task = worker_thread->interrupt_task_list_head;
            while (interrupt_task) {
                if (interrupt_task->triggered && interrupt_task->task_func) {
                    interrupt_task->triggered = false;
                    interrupt_task->task_func(interrupt_task);
                }
                chDbgCheck(interrupt_task->next != interrupt_task);
                interrupt_task = interrupt_task->next;
            }
            chMtxUnlockAll();
        }

#ifdef MODULE_PUBSUB_ENABLED
        // Check for immediately available messages on listener tasks, handle one
        {
            chMtxLock(&worker_thread->mtx);
            struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
            while (listener_task) {
                if (pubsub_listener_handle_one_timeout(listener_task->listener, TIME_IMMEDIATE)) {
                    break;
                }
                chDbgCheck(listener_task->next != listener_task);
                listener_task = listener_task->next;
            }
            chMtxUnlockAll();
        }
#endif

        chMtxLock(&worker_thread->mtx);

        systime_t tnow_ticks = chVTGetSystemTimeX();
        systime_t ticks_to_next_timer_task = worker_thread_get_ticks_to_next_timer_task(worker_thread, tnow_ticks);
        if (ticks_to_next_timer_task == TIME_IMMEDIATE) {
            // Task is due - pop the task off the task list, run it, reschedule if task is auto-repeat
            struct worker_thread_timer_task_s* next_timer_task = worker_thread->next_timer_task;
            worker_thread->next_timer_task = next_timer_task->next;

            chMtxUnlockAll();

            // Perform task
            next_timer_task->task_func(next_timer_task);
            next_timer_task->last_run_time_ticks = tnow_ticks;

            if (next_timer_task->auto_repeat) {
                // Re-insert task
                chMtxLock(&worker_thread->mtx);
                worker_thread_insert_timer_task(worker_thread, next_timer_task);
                chMtxUnlockAll();
            }
        } else {
            // No task due - go to sleep until there is a task
            chSysLock();
            chMtxUnlockAllS();

            // If an interrupt task is due, we should not sleep until we've handled it
            if (worker_thread_get_interrupt_task_triggered_S(worker_thread)) {
                chSysUnlock();
                continue;
            }

#ifdef MODULE_PUBSUB_ENABLED
            // If a listener task is due, we should not sleep until we've handled it
            if (worker_thread_get_listener_task_waiting_S(worker_thread)) {
                chSysUnlock();
                continue;
            }

            thread_reference_t trp = NULL;
            worker_thread_set_listener_thread_references_S(worker_thread, &trp);
#endif

            worker_thread->waiting = true;
            chThdSuspendTimeoutS(&trp, ticks_to_next_timer_task);
            worker_thread->waiting = false;

#ifdef MODULE_PUBSUB_ENABLED
            worker_thread_set_listener_thread_references_S(worker_thread, NULL);
#endif

            chSysUnlock();
        }
    }
}

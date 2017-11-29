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
    worker_thread->publisher_task_list_head = NULL;
#endif
    worker_thread->waiting = false;
    chMtxObjectInit(&worker_thread->mtx);
}

static void wake_worker_thread(struct worker_thread_s* worker_thread) {
    chSysLock();
    if (worker_thread->waiting) {
        chSchWakeupS(worker_thread->thread, MSG_TIMEOUT);
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
    chMtxUnlock(&worker_thread->mtx);

    // Wake worker thread to process tasks
    wake_worker_thread(worker_thread);
}

void worker_thread_remove_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    chMtxLock(&worker_thread->mtx);

    LINKED_LIST_REMOVE(struct worker_thread_timer_task_s, worker_thread->next_timer_task, task);

    chMtxUnlock(&worker_thread->mtx);
}

void* worker_thread_task_get_user_context(struct worker_thread_timer_task_s* task) {
    if (!task) {
        return NULL;
    }

    return task->ctx;
}

#ifdef MODULE_PUBSUB_ENABLED
void worker_thread_add_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task, struct pubsub_topic_s* topic, pubsub_message_handler_func_ptr handler_cb, void* handler_cb_ctx) {
    pubsub_listener_init_and_register(&task->listener, topic, handler_cb, handler_cb_ctx);

    chMtxLock(&worker_thread->mtx);
    LINKED_LIST_APPEND(struct worker_thread_listener_task_s, worker_thread->listener_task_list_head, task);
    chMtxUnlock(&worker_thread->mtx);

    // Wake worker thread to process tasks
    wake_worker_thread(worker_thread);
}

void worker_thread_remove_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task) {
    pubsub_listener_unregister(&task->listener);

    chMtxLock(&worker_thread->mtx);
    LINKED_LIST_REMOVE(struct worker_thread_listener_task_s, worker_thread->listener_task_list_head, task);
    chMtxUnlock(&worker_thread->mtx);
}

void worker_thread_add_publisher_task(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* task, struct pubsub_topic_s* topic, size_t msg_max_size, size_t msg_queue_depth) {
    size_t mem_block_size = sizeof(struct worker_thread_publisher_msg_s)+msg_max_size;

    task->topic = topic;
    task->msg_max_size = msg_max_size;
    chPoolObjectInit(&task->pool, mem_block_size, NULL);
    chMBObjectInit(&task->mailbox, chCoreAllocAligned(sizeof(msg_t)*msg_queue_depth, PORT_WORKING_AREA_ALIGN), msg_queue_depth);
    task->worker_thread = worker_thread;

    chPoolLoadArray(&task->pool, chCoreAllocAligned(mem_block_size*msg_queue_depth, PORT_WORKING_AREA_ALIGN), msg_queue_depth);

    chMtxLock(&worker_thread->mtx);
    LINKED_LIST_APPEND(struct worker_thread_publisher_task_s, worker_thread->publisher_task_list_head, task);
    chMtxUnlock(&worker_thread->mtx);
}

void worker_thread_remove_publisher_task(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* task) {
    chMtxLock(&worker_thread->mtx);
    LINKED_LIST_REMOVE(struct worker_thread_publisher_task_s, worker_thread->publisher_task_list_head, task);
    chMtxUnlock(&worker_thread->mtx);
}

bool worker_thread_publisher_task_publish_I(struct worker_thread_publisher_task_s* task, size_t size, pubsub_message_writer_func_ptr writer_cb, void* ctx) {
    if (size > task->msg_max_size) {
        return false;
    }

    struct worker_thread_publisher_msg_s* msg = chPoolAllocI(&task->pool);

    if (!msg) {
        return false;
    }

    if (writer_cb) {
        writer_cb(size, msg->data, ctx);
    }

    chMBPostI(&task->mailbox, (msg_t)msg);

    if (task->worker_thread->waiting) {
        task->worker_thread->thread->u.rdymsg = MSG_TIMEOUT;
        chSchReadyI(task->worker_thread->thread);
        task->worker_thread->waiting = false;
    }
    return true;
}

static bool worker_thread_get_any_publisher_task_due_S(struct worker_thread_s* worker_thread) {
    struct worker_thread_publisher_task_s* task = worker_thread->publisher_task_list_head;
    while (task) {
        if (chMBGetUsedCountI(&task->mailbox) != 0) {
            return true;
        }
        chDbgCheck(task->next != task);
        task = task->next;
    }
    return false;
}

static void worker_thread_set_listener_thread_references_S(struct worker_thread_s* worker_thread, thread_reference_t* trpp) {
    struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
    while (listener_task) {
        pubsub_listener_set_waiting_thread_reference_S(&listener_task->listener, trpp);
        listener_task = listener_task->next;
    }
}

static bool worker_thread_get_any_listener_task_due_S(struct worker_thread_s* worker_thread) {
    struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
    while (listener_task) {
        if (pubsub_listener_has_message(&listener_task->listener)) {
            return true;
        }
        chDbgCheck(listener_task->next != listener_task);
        listener_task = listener_task->next;
    }
    return false;
}
#endif

static void worker_thread_insert_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    systime_t task_run_time = task->last_run_time_ticks + task->period_ticks;
    struct worker_thread_timer_task_s** insert_ptr = &worker_thread->next_timer_task;
    while (*insert_ptr && task_run_time - (*insert_ptr)->last_run_time_ticks >= (*insert_ptr)->period_ticks) {
        insert_ptr = &(*insert_ptr)->next;
    }
    task->next = *insert_ptr;
    *insert_ptr = task;
}

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
#ifdef MODULE_PUBSUB_ENABLED
        // Handle publisher tasks
        {
            chMtxLock(&worker_thread->mtx);
            struct worker_thread_publisher_task_s* task = worker_thread->publisher_task_list_head;
            while (task) {
                struct worker_thread_publisher_msg_s* msg;
                while (chMBFetch(&task->mailbox, (msg_t*)&msg, TIME_IMMEDIATE) == MSG_OK) {
                    pubsub_publish_message(task->topic, msg->size, pubsub_copy_writer_func, msg->data);
                    chPoolFree(&task->pool, msg);
                }
                task = task->next;
            }
            chMtxUnlockAll();
        }

        // Check for immediately available messages on listener tasks, handle one
        {
            chMtxLock(&worker_thread->mtx);
            struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
            while (listener_task) {
                if (pubsub_listener_handle_one_timeout(&listener_task->listener, TIME_IMMEDIATE)) {
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

#ifdef MODULE_PUBSUB_ENABLED
            // If a listener task is due, we should not sleep until we've handled it
            if (worker_thread_get_any_listener_task_due_S(worker_thread)) {
                chSysUnlock();
                continue;
            }

            // If a publisher task is due, we should not sleep until we've handled it
            if (worker_thread_get_any_publisher_task_due_S(worker_thread)) {
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

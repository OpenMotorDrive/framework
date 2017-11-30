#include "worker_thread.h"

#include <common/helpers.h>

static THD_FUNCTION(worker_thread_func, arg);

static void worker_thread_wake_I(struct worker_thread_s* worker_thread);
static void worker_thread_reschedule_S(struct worker_thread_s* worker_thread);
static void worker_thread_wake(struct worker_thread_s* worker_thread);
static void worker_thread_init_timer_task(struct worker_thread_timer_task_s* task, systime_t timer_begin_systime, systime_t timer_expiration_ticks, bool auto_repeat, timer_task_handler_func_ptr task_func, void* ctx);
static void worker_thread_insert_timer_task_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task);
static systime_t worker_thread_get_ticks_to_timer_task_I(struct worker_thread_timer_task_s* task, systime_t tnow_ticks);
static bool worker_thread_timer_task_is_registered_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* check_task);
#ifdef MODULE_PUBSUB_ENABLED
static bool worker_thread_publisher_task_is_registered_I(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* check_task);
static bool worker_thread_publisher_task_is_registered(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* check_task);
static bool worker_thread_get_any_publisher_task_due(struct worker_thread_s* worker_thread);
static bool worker_thread_get_any_publisher_task_due_I(struct worker_thread_s* worker_thread);
static bool worker_thread_listener_task_is_registered_I(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* check_task);
static bool worker_thread_listener_task_is_registered(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* check_task);
static void worker_thread_set_listener_thread_references_S(struct worker_thread_s* worker_thread, thread_reference_t* trpp);
static bool worker_thread_get_any_listener_task_due_I(struct worker_thread_s* worker_thread);
#endif


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
    worker_thread->timer_task_list_head = NULL;
#ifdef MODULE_PUBSUB_ENABLED
    worker_thread->listener_task_list_head = NULL;
    worker_thread->publisher_task_list_head = NULL;
#endif
}

void worker_thread_add_timer_task_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, timer_task_handler_func_ptr task_func, void* ctx, systime_t timer_expiration_ticks, bool auto_repeat) {
    chDbgCheckClassI();

    worker_thread_init_timer_task(task, chVTGetSystemTimeX(), timer_expiration_ticks, auto_repeat, task_func, ctx);
    worker_thread_insert_timer_task_I(worker_thread, task);

    // Wake worker thread to process tasks
    worker_thread_wake_I(worker_thread);
}

void worker_thread_add_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, timer_task_handler_func_ptr task_func, void* ctx, systime_t timer_expiration_ticks, bool auto_repeat) {
    chSysLock();
    worker_thread_add_timer_task_I(worker_thread, task, task_func, ctx, timer_expiration_ticks, auto_repeat);
    worker_thread_reschedule_S(worker_thread);
    chSysUnlock();
}

void worker_thread_timer_task_reschedule_earlier_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, systime_t timer_expiration_ticks) {
    chDbgCheckClassI();

    systime_t t_now = chVTGetSystemTimeX();

    if (worker_thread_timer_task_is_registered_I(worker_thread, task)) {
        if (worker_thread_get_ticks_to_timer_task_I(task, t_now) <= timer_expiration_ticks) {
            return;
        }

        worker_thread_remove_timer_task_I(worker_thread, task);
    }

    task->timer_expiration_ticks = timer_expiration_ticks;
    task->timer_begin_systime = t_now;

    worker_thread_insert_timer_task_I(worker_thread, task);
    worker_thread_wake_I(worker_thread);
}

void worker_thread_timer_task_reschedule_earlier(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, systime_t timer_expiration_ticks) {
    chSysLock();
    worker_thread_timer_task_reschedule_earlier_I(worker_thread, task, timer_expiration_ticks);
    worker_thread_reschedule_S(worker_thread);
    chSysUnlock();
}

void worker_thread_remove_timer_task_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    chDbgCheckClassI();
    LINKED_LIST_REMOVE(struct worker_thread_timer_task_s, worker_thread->timer_task_list_head, task);
}

void worker_thread_remove_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    chSysLock();
    worker_thread_remove_timer_task_I(worker_thread, task);
    chSysUnlock();
}

void* worker_thread_task_get_user_context(struct worker_thread_timer_task_s* task) {
    if (!task) {
        return NULL;
    }

    return task->ctx;
}

#ifdef MODULE_PUBSUB_ENABLED
void worker_thread_add_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task, struct pubsub_topic_s* topic, pubsub_message_handler_func_ptr handler_cb, void* handler_cb_ctx) {
    chDbgCheck(!worker_thread_listener_task_is_registered(worker_thread, task));

    pubsub_listener_init_and_register(&task->listener, topic, handler_cb, handler_cb_ctx);

    chSysLock();
    LINKED_LIST_APPEND(struct worker_thread_listener_task_s, worker_thread->listener_task_list_head, task);
    chSysUnlock();

    // Wake worker thread to process tasks
    worker_thread_wake(worker_thread);
}

void worker_thread_remove_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task) {
    pubsub_listener_unregister(&task->listener);

    chSysLock();
    LINKED_LIST_REMOVE(struct worker_thread_listener_task_s, worker_thread->listener_task_list_head, task);
    chSysUnlock();
}

void worker_thread_add_publisher_task(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* task, struct pubsub_topic_s* topic, size_t msg_max_size, size_t msg_queue_depth) {
    chDbgCheck(!worker_thread_publisher_task_is_registered(worker_thread, task));
    size_t mem_block_size = sizeof(struct worker_thread_publisher_msg_s)+msg_max_size;

    task->topic = topic;
    task->msg_max_size = msg_max_size;
    chPoolObjectInit(&task->pool, mem_block_size, NULL);
    chMBObjectInit(&task->mailbox, chCoreAllocAligned(sizeof(msg_t)*msg_queue_depth, PORT_WORKING_AREA_ALIGN), msg_queue_depth);
    task->worker_thread = worker_thread;

    chPoolLoadArray(&task->pool, chCoreAllocAligned(mem_block_size*msg_queue_depth, PORT_WORKING_AREA_ALIGN), msg_queue_depth);

    chSysLock();
    LINKED_LIST_APPEND(struct worker_thread_publisher_task_s, worker_thread->publisher_task_list_head, task);
    chSysUnlock();
}

void worker_thread_remove_publisher_task(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* task) {
    chSysLock();
    LINKED_LIST_REMOVE(struct worker_thread_publisher_task_s, worker_thread->publisher_task_list_head, task);
    chSysUnlock();
}

bool worker_thread_publisher_task_publish_from_ISR(struct worker_thread_publisher_task_s* task, size_t size, pubsub_message_writer_func_ptr writer_cb, void* ctx) {
    if (size > task->msg_max_size) {
        return false;
    }

    chSysLockFromISR();
    struct worker_thread_publisher_msg_s* msg = chPoolAllocI(&task->pool);
    chSysUnlockFromISR();

    if (!msg) {
        return false;
    }

    msg->size = size;

    if (writer_cb) {
        writer_cb(size, msg->data, ctx);
    }

    chSysLockFromISR();
    chMBPostI(&task->mailbox, (msg_t)msg);

    worker_thread_wake_I(task->worker_thread);
    chSysUnlockFromISR();
    return true;
}
#endif

static THD_FUNCTION(worker_thread_func, arg) {
    struct worker_thread_s* worker_thread = arg;
    if (worker_thread->name) {
        chRegSetThreadName(worker_thread->name);
    }

    while (true) {
#ifdef MODULE_PUBSUB_ENABLED
        // Handle publisher tasks
        {
            chSysLock();
            struct worker_thread_publisher_task_s* task = worker_thread->publisher_task_list_head;
            chSysUnlock();
            while (task) {
                struct worker_thread_publisher_msg_s* msg;
                while (chMBFetch(&task->mailbox, (msg_t*)&msg, TIME_IMMEDIATE) == MSG_OK) {
                    pubsub_publish_message(task->topic, msg->size, pubsub_copy_writer_func, msg->data);
                    chPoolFree(&task->pool, msg);
                }
                chSysLock();
                task = task->next;
                chSysUnlock();
            }
        }

        // Check for immediately available messages on listener tasks, handle one
        {
            chSysLock();
            struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
            chSysUnlock();
            while (listener_task) {
                if (pubsub_listener_handle_one_timeout(&listener_task->listener, TIME_IMMEDIATE)) {
                    break;
                }
                chSysLock();
                listener_task = listener_task->next;
                chSysUnlock();
            }
        }
#endif
        chSysLock();
        systime_t tnow_ticks = chVTGetSystemTimeX();
        systime_t ticks_to_next_timer_task = worker_thread_get_ticks_to_timer_task_I(worker_thread->timer_task_list_head, tnow_ticks);
        if (ticks_to_next_timer_task == TIME_IMMEDIATE) {
            // Task is due - pop the task off the task list, run it, reschedule if task is auto-repeat
            struct worker_thread_timer_task_s* next_timer_task = worker_thread->timer_task_list_head;
            worker_thread->timer_task_list_head = next_timer_task->next;

            chSysUnlock();

            // Perform task
            next_timer_task->task_func(next_timer_task);
            next_timer_task->timer_begin_systime = tnow_ticks;

            if (next_timer_task->auto_repeat) {
                // Re-insert task
                chSysLock();
                worker_thread_insert_timer_task_I(worker_thread, next_timer_task);
                chSysUnlock();
            }
        } else {
#ifdef MODULE_PUBSUB_ENABLED
            // If a listener task is due, we should not sleep until we've handled it
            if (worker_thread_get_any_listener_task_due_I(worker_thread)) {
                chSysUnlock();
                continue;
            }

            // If a publisher task is due, we should not sleep until we've handled it
            if (worker_thread_get_any_publisher_task_due_I(worker_thread)) {
                chSysUnlock();
                continue;
            }

            thread_reference_t trp = NULL;
            worker_thread_set_listener_thread_references_S(worker_thread, &trp);
#endif

            // No task due - go to sleep until there is a task
            chThdSuspendTimeoutS(&trp, ticks_to_next_timer_task);

#ifdef MODULE_PUBSUB_ENABLED
            worker_thread_set_listener_thread_references_S(worker_thread, NULL);
#endif

            chSysUnlock();
        }
    }
}

static void worker_thread_wake_I(struct worker_thread_s* worker_thread) {
    chDbgCheckClassI();

    if (worker_thread->thread->state == CH_STATE_SUSPENDED) {
        worker_thread->thread->u.rdymsg = MSG_TIMEOUT;
        chSchReadyI(worker_thread->thread);
    }
}

static void worker_thread_reschedule_S(struct worker_thread_s* worker_thread) {
    chDbgCheckClassS();

    if (worker_thread->thread->state == CH_STATE_READY && chThdGetPriorityX() < worker_thread->thread->prio) {
        chSchRescheduleS();
    }
}

static void worker_thread_wake(struct worker_thread_s* worker_thread) {
    chSysLock();
    worker_thread_wake_I(worker_thread);
    worker_thread_reschedule_S(worker_thread);
    chSysUnlock();
}

static void worker_thread_init_timer_task(struct worker_thread_timer_task_s* task, systime_t timer_begin_systime, systime_t timer_expiration_ticks, bool auto_repeat, timer_task_handler_func_ptr task_func, void* ctx) {
    task->task_func = task_func;
    task->ctx = ctx;
    task->timer_expiration_ticks = timer_expiration_ticks;
    task->auto_repeat = auto_repeat;
    task->timer_begin_systime = timer_begin_systime;
}

static bool worker_thread_timer_task_is_registered_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* check_task) {
    chDbgCheckClassI();

    struct worker_thread_timer_task_s* task = worker_thread->timer_task_list_head;
    while (task) {
        if (task == check_task) {
            return true;
        }
        task = task->next;
    }
    return false;
}

static void worker_thread_insert_timer_task_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    chDbgCheckClassI();
    chDbgCheck(!worker_thread_timer_task_is_registered_I(worker_thread, task));

    if (task->timer_expiration_ticks == TIME_INFINITE) {
        return;
    }

    systime_t task_run_time = task->timer_begin_systime + task->timer_expiration_ticks;
    struct worker_thread_timer_task_s** insert_ptr = &worker_thread->timer_task_list_head;
    while (*insert_ptr && task_run_time - (*insert_ptr)->timer_begin_systime >= (*insert_ptr)->timer_expiration_ticks) {
        insert_ptr = &(*insert_ptr)->next;
    }
    task->next = *insert_ptr;
    *insert_ptr = task;
}

static systime_t worker_thread_get_ticks_to_timer_task_I(struct worker_thread_timer_task_s* task, systime_t tnow_ticks) {
    chDbgCheckClassI();

    if (task && task->timer_expiration_ticks != TIME_INFINITE) {
        systime_t elapsed = tnow_ticks - task->timer_begin_systime;
        if (elapsed >= task->timer_expiration_ticks) {
            return TIME_IMMEDIATE;
        } else {
            return task->timer_expiration_ticks - elapsed;
        }
    } else {
        return TIME_INFINITE;
    }
}

#ifdef MODULE_PUBSUB_ENABLED
static bool worker_thread_publisher_task_is_registered_I(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* check_task) {
    chDbgCheckClassI();

    struct worker_thread_publisher_task_s* task = worker_thread->publisher_task_list_head;
    while (task) {
        if (task == check_task) {
            return true;
        }
        task = task->next;
    }
    return false;
}

static bool worker_thread_publisher_task_is_registered(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* check_task) {
    chSysLock();
    bool ret = worker_thread_publisher_task_is_registered_I(worker_thread, check_task);
    chSysUnlock();
    return ret;
}

static bool worker_thread_get_any_publisher_task_due_I(struct worker_thread_s* worker_thread) {
    chDbgCheckClassI();

    struct worker_thread_publisher_task_s* task = worker_thread->publisher_task_list_head;
    while (task) {
        if (chMBGetUsedCountI(&task->mailbox) != 0) {
            return true;
        }
        task = task->next;
    }
    return false;
}

static bool worker_thread_listener_task_is_registered_I(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* check_task) {
    chDbgCheckClassI();

    struct worker_thread_listener_task_s* task = worker_thread->listener_task_list_head;
    while (task) {
        if (task == check_task) {
            return true;
        }
        task = task->next;
    }
    return false;
}

static bool worker_thread_listener_task_is_registered(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* check_task) {
    chSysLock();
    bool ret = worker_thread_listener_task_is_registered_I(worker_thread, check_task);
    chSysUnlock();
    return ret;
}

static void worker_thread_set_listener_thread_references_S(struct worker_thread_s* worker_thread, thread_reference_t* trpp) {
    chDbgCheckClassS();

    struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
    while (listener_task) {
        pubsub_listener_set_waiting_thread_reference_S(&listener_task->listener, trpp);
        listener_task = listener_task->next;
    }
}

static bool worker_thread_get_any_listener_task_due_I(struct worker_thread_s* worker_thread) {
    chDbgCheckClassI();

    struct worker_thread_listener_task_s* listener_task = worker_thread->listener_task_list_head;
    while (listener_task) {
        if (pubsub_listener_has_message(&listener_task->listener)) {
            return true;
        }
        listener_task = listener_task->next;
    }
    return false;
}
#endif

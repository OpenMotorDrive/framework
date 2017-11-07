#include "worker_thread.h"

static THD_FUNCTION(worker_thread_func, arg);
static void worker_thread_insert_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task);

void worker_thread_init(struct worker_thread_s* worker_thread, size_t stack_size, tprio_t prio) {
    if (!worker_thread) {
        return;
    }
    void* working_area = chCoreAllocAligned(THD_WORKING_AREA_SIZE(stack_size), PORT_WORKING_AREA_ALIGN);
    if (!working_area) {
        return;
    }
    worker_thread->thread = chThdCreateStatic(working_area, THD_WORKING_AREA_SIZE(stack_size), prio, worker_thread_func, worker_thread);
    worker_thread->next_timer_task = NULL;
    worker_thread->waiting = false;
    chMtxObjectInit(&worker_thread->mtx);
}

void worker_thread_add_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, task_handler_func_ptr task_func, void* ctx, systime_t period_ticks, bool auto_repeat) {
    task->task_func = task_func;
    task->ctx = ctx;
    task->period_ticks = period_ticks;
    task->auto_repeat = auto_repeat;
    task->last_run_time_ticks = chVTGetSystemTimeX();

    chMtxLock(&worker_thread->mtx);
    worker_thread_insert_task(worker_thread, task);
    chMtxUnlock(&worker_thread->mtx);

    // Wake worker thread to process tasks
    chSysLock();
    if (worker_thread->waiting) {
        chSchWakeupS(worker_thread->thread, MSG_TIMEOUT);
    }
    chSysUnlock();
}

static void worker_thread_insert_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task) {
    systime_t task_run_time = task->last_run_time_ticks + task->period_ticks;
    struct worker_thread_timer_task_s** insert_ptr = &worker_thread->next_timer_task;
    while (*insert_ptr && task_run_time - (*insert_ptr)->last_run_time_ticks >= (*insert_ptr)->period_ticks) {
        insert_ptr = &(*insert_ptr)->next;
    }
    task->next = *insert_ptr;
    *insert_ptr = task;
}

static THD_FUNCTION(worker_thread_func, arg) {
    struct worker_thread_s* worker_thread = arg;

    while (true) {
        systime_t tnow_ticks = chVTGetSystemTimeX();
        systime_t timeout;

        chMtxLock(&worker_thread->mtx);
        while (true) {
            struct worker_thread_timer_task_s* next_timer_task = worker_thread->next_timer_task;
            if (next_timer_task) {
                // Check if task is due
                systime_t elapsed = tnow_ticks - next_timer_task->last_run_time_ticks;
                if (elapsed >= next_timer_task->period_ticks) {
                    // Task is due - pop the task off the task list
                    worker_thread->next_timer_task = next_timer_task->next;

                    // Perform task
                    next_timer_task->task_func(next_timer_task->ctx);
                    next_timer_task->last_run_time_ticks = tnow_ticks;

                    if (next_timer_task->auto_repeat) {
                        // Re-insert task
                        worker_thread_insert_task(worker_thread, next_timer_task);
                    }
                } else {
                    // Task is not due
                    timeout = next_timer_task->period_ticks - elapsed;
                    break;
                }
            } else {
                timeout = TIME_INFINITE;
                break;
            }

            chMtxUnlock(&worker_thread->mtx);
            chMtxLock(&worker_thread->mtx);
        }

        chSysLock();
        chMtxUnlockS(&worker_thread->mtx);
        worker_thread->waiting = true;
        chThdSleepS(timeout);
        worker_thread->waiting = false;
        chSysUnlock();
    }
}

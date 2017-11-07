#pragma once

#include <ch.h>

typedef void (*task_handler_func_ptr)(void* ctx);

struct worker_thread_timer_task_s {
    task_handler_func_ptr task_func;
    void* ctx;
    systime_t period_ticks;
    systime_t last_run_time_ticks;
    bool auto_repeat;
    struct worker_thread_timer_task_s* next;
};

struct worker_thread_s {
    thread_t* thread;
    bool waiting;
    struct worker_thread_timer_task_s* next_timer_task;
    mutex_t mtx;
};

void worker_thread_init(struct worker_thread_s* worker_thread, size_t stack_size, tprio_t priority);
void worker_thread_add_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, task_handler_func_ptr task_func, void* ctx, systime_t period_ticks, bool auto_repeat);

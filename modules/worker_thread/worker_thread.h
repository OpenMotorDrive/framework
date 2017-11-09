#pragma once

#include <ch.h>
#include <common/ctor.h>

#ifdef MODULE_PUBSUB_ENABLED
#include <pubsub/pubsub.h>
#endif

#define __WORKER_THREAD_CONCAT(a,b) a ## b
#define _WORKER_THREAD_CONCAT(a,b) __WORKER_THREAD_CONCAT(a,b)

#define WORKER_THREAD_PERIODIC_TIMER_TASK_AUTOSTART(TASK_NAME, WORKER_THREAD, PERIOD) \
static struct worker_thread_timer_task_s TASK_NAME; \
static void _WORKER_THREAD_CONCAT(TASK_NAME,_handler_func)(struct worker_thread_timer_task_s* task); \
RUN_ON(INIT_END) { \
    worker_thread_add_timer_task(WORKER_THREAD, &TASK_NAME, _WORKER_THREAD_CONCAT(TASK_NAME,_handler_func), NULL, PERIOD, true); \
} \
static void _WORKER_THREAD_CONCAT(TASK_NAME,_handler_func)(struct worker_thread_timer_task_s* task)

struct worker_thread_timer_task_s;
struct worker_thread_s;

typedef void (*task_handler_func_ptr)(struct worker_thread_timer_task_s* task);

struct worker_thread_timer_task_s {
    task_handler_func_ptr task_func;
    void* ctx;
    systime_t period_ticks;
    systime_t last_run_time_ticks;
    bool auto_repeat;
    struct worker_thread_timer_task_s* next;
};

#ifdef MODULE_PUBSUB_ENABLED
struct worker_thread_listener_task_s {
    struct pubsub_listener_s* listener;
    struct worker_thread_listener_task_s* next;
};
#endif

struct worker_thread_s {
    thread_t* thread;
    bool waiting;
    struct worker_thread_timer_task_s* next_timer_task;
#ifdef MODULE_PUBSUB_ENABLED
    struct worker_thread_listener_task_s* listener_task_list_head;
#endif
    mutex_t mtx;
};

void worker_thread_init(struct worker_thread_s* worker_thread, size_t stack_size, tprio_t priority);
void worker_thread_add_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, task_handler_func_ptr task_func, void* ctx, systime_t period_ticks, bool auto_repeat);
void worker_thread_remove_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task);
void* worker_thread_task_get_user_context(struct worker_thread_timer_task_s* task);
#ifdef MODULE_PUBSUB_ENABLED
void worker_thread_add_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task, struct pubsub_listener_s* listener);
void worker_thread_remove_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task);
#endif

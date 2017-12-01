#pragma once

#include <ch.h>
#include <common/ctor.h>

#ifdef MODULE_PUBSUB_ENABLED
#include <modules/pubsub/pubsub.h>
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
struct worker_thread_interrupt_task_s;
struct worker_thread_s;

typedef void (*timer_task_handler_func_ptr)(struct worker_thread_timer_task_s* task);

struct worker_thread_timer_task_s {
    timer_task_handler_func_ptr task_func;
    void* ctx;
    systime_t timer_expiration_ticks;
    systime_t timer_begin_systime;
    bool auto_repeat;
    struct worker_thread_timer_task_s* next;
};

#ifdef MODULE_PUBSUB_ENABLED
struct worker_thread_listener_task_s {
    struct pubsub_listener_s listener;
    struct worker_thread_listener_task_s* next;
};

struct worker_thread_publisher_msg_s {
    size_t size;
    uint8_t data[];
};

struct worker_thread_publisher_task_s {
    struct pubsub_topic_s* topic;
    size_t msg_max_size;
    memory_pool_t pool;
    mailbox_t mailbox;
    struct worker_thread_s* worker_thread;
    struct worker_thread_publisher_task_s* next;
};
#endif

struct worker_thread_s {
    thread_t* thread;
    const char* name;
    struct worker_thread_timer_task_s* timer_task_list_head;
#ifdef MODULE_PUBSUB_ENABLED
    struct worker_thread_listener_task_s* listener_task_list_head;
    struct worker_thread_publisher_task_s* publisher_task_list_head;
#endif
};

void worker_thread_init(struct worker_thread_s* worker_thread, const char* name, size_t stack_size, tprio_t priority);
void worker_thread_add_timer_task_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, timer_task_handler_func_ptr task_func, void* ctx, systime_t timer_expiration_ticks, bool auto_repeat);
void worker_thread_add_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, timer_task_handler_func_ptr task_func, void* ctx, systime_t timer_expiration_ticks, bool auto_repeat);
void worker_thread_timer_task_reschedule_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, systime_t timer_expiration_ticks);
void worker_thread_timer_task_reschedule(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task, systime_t timer_expiration_ticks);
void worker_thread_remove_timer_task_I(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task);
void worker_thread_remove_timer_task(struct worker_thread_s* worker_thread, struct worker_thread_timer_task_s* task);
void* worker_thread_task_get_user_context(struct worker_thread_timer_task_s* task);
#ifdef MODULE_PUBSUB_ENABLED
void worker_thread_add_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task, struct pubsub_topic_s* topic, pubsub_message_handler_func_ptr handler_cb, void* handler_cb_ctx);
void worker_thread_remove_listener_task(struct worker_thread_s* worker_thread, struct worker_thread_listener_task_s* task);
void worker_thread_add_publisher_task(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* task, struct pubsub_topic_s* topic, size_t msg_max_size, size_t msg_queue_depth);
void worker_thread_remove_publisher_task(struct worker_thread_s* worker_thread, struct worker_thread_publisher_task_s* task);
bool worker_thread_publisher_task_publish_I(struct worker_thread_publisher_task_s* task, size_t size, pubsub_message_writer_func_ptr writer_cb, void* ctx);
#endif

#pragma once

#include <stddef.h>
#include <common/fifoallocator.h>
#include <ch.h>

#define __PUBSUB_CONCAT(a,b) a ## b
#define _PUBSUB_CONCAT(a,b) __PUBSUB_CONCAT(a,b)

#define OMD_PUBSUB_DECLARE_TOPIC_GROUP_STATIC(HANDLE_NAME, SIZE) \
static struct pubsub_topic_group_s HANDLE_NAME; \
static uint8_t _PUBSUB_CONCAT(_pubsub_topic_group_memory_, HANDLE_NAME)[SIZE]; \
RUN_BEFORE(OMD_PUBSUB_TOPIC_INIT) { \
    pubsub_create_topic_group(&HANDLE_NAME, SIZE, _PUBSUB_CONCAT(_pubsub_topic_group_memory_, HANDLE_NAME)); \
}

typedef void (*pubsub_message_writer_func_ptr)(size_t msg_size, void* msg, void* ctx);
typedef void (*pubsub_message_handler_func_ptr)(size_t msg_size, const void* msg, void* ctx);

struct pubsub_message_s;
struct pubsub_topic_s;
struct pubsub_listener_s;
struct pubsub_topic_group_s;

struct pubsub_message_s {
    struct pubsub_topic_s* topic;
    struct pubsub_message_s* next_in_topic;
    uint8_t data[];
};

struct pubsub_listener_s {
    struct pubsub_topic_s* topic;
    struct pubsub_message_s* next_message;
    thread_reference_t* waiting_thread_reference_ptr;
    pubsub_message_handler_func_ptr handler_cb;
    void* handler_cb_ctx;
    mutex_t mtx;
    struct pubsub_listener_s* next;
};

struct pubsub_topic_s {
    struct pubsub_message_s* message_list_tail;
    struct pubsub_topic_group_s* group;
    struct pubsub_listener_s* listener_list_head;
};

struct pubsub_topic_group_s {
    struct fifoallocator_instance_s allocator;
    mutex_t mtx;
};

// - Creates a new topic group with a separate memory pool and mutex. This new topic group is insulated from problems on
//   other topic groups, e.g. badly-behaved listeners locking the allocator and blocking for too long or publishers
//   publishing messages that are too large, causing the allocator to deallocate every other message.
void pubsub_create_topic_group(struct pubsub_topic_group_s* topic_group, size_t memory_pool_size, void* memory_pool);

// - Initializes a new topic object.
// - topic_group may be NULL, in which case the default topic group is used.
void pubsub_init_topic(struct pubsub_topic_s* topic, struct pubsub_topic_group_s* topic_group);

// - Initializes a listener object owned by the current thread and registers it on a topic.
void pubsub_init_and_register_listener(struct pubsub_topic_s* topic, struct pubsub_listener_s* listener);

// - Allocates a message on topic topic of size size, calls writer_cb(size, msg, ctx) to populate it, and publishes it.
void pubsub_publish_message(struct pubsub_topic_s* topic, size_t size, pubsub_message_writer_func_ptr writer_cb, void* ctx);

// - Unregisters a listener from its topic.
void pubsub_listener_unregister(struct pubsub_listener_s* listener);

// - Resets a listener to a state of no messages pending.
void pubsub_listener_reset(struct pubsub_listener_s* listener);

// - Sets the handler callback and context variable that it will be called with. Note that the handler callback will not be called
//   until the listener's owner thread calls one of the following APIs:
//     - pubsub_listener_handle_until_timeout
//     - pubsub_multiple_listener_handle_until_timeout
// - Note that while handler_cb is executing, publishers on the listener's topic group can be blocked if they need to deallocate
//   the message that the handler is handling. This problem can be mitigated by minimizing blocking, allocating more memory to the
//   topic group, or using a separate topic group.
void pubsub_listener_set_handler_cb(struct pubsub_listener_s* listener, pubsub_message_handler_func_ptr handler_cb, void* handler_cb_ctx);

// - Handles messages that become available to listener using the listener's handler_cb. Returns after timeout has elapsed.
// - Note that a listener is intended to have one owner thread, and calling this on the same listener from multiple threads is forbidden.
// - timeout can be TIME_IMMEDIATE to handle all immediately available messages, or TIME_INFINITE to handle messages forever.
// - Guarantees chronological handling of messages.
void pubsub_listener_handle_until_timeout(struct pubsub_listener_s* listener, systime_t timeout);

// - Handles messages that become available to any listener in the listeners array using the listener's handler_cb. Returns after timeout has elapsed.
// - Note that a listener is intended to have a single owner thread, and calling this function on the same listener from multiple threads is forbidden.
// - timeout can be TIME_IMMEDIATE to handle all immediately available messages, or TIME_INFINITE to handle messages forever.
// - Guarantees chronological handling of messages within the same topic. Does *not* guarantee chronological handling of messages across the topic list.
void pubsub_multiple_listener_handle_until_timeout(size_t num_listeners, struct pubsub_listener_s** listeners, systime_t timeout);

#pragma once
#include <stddef.h>
#include <common/fifoallocator.h>
#include <ch.h>

typedef void (*pubsub_message_writer_func_ptr)(size_t msg_size, void* msg, void* ctx);
typedef void (*pubsub_message_handler_func_ptr)(size_t msg_size, const void* msg, void* ctx);

struct pubsub_message_s;
struct pubsub_topic_s;
struct pubsub_listener_s;

struct pubsub_message_s {
    struct pubsub_topic_s* topic;
    struct pubsub_message_s* next_in_topic;
    uint8_t data[];
};

struct pubsub_listener_s {
    struct pubsub_topic_s* topic;
    struct pubsub_message_s* next_message;
    thread_t* owner_thread;
    mutex_t listener_mtx;
    struct pubsub_listener_s* next;
};

struct pubsub_topic_s {
    const char* name;
    struct pubsub_message_s* message_list_tail;
    struct pubsub_topic_group_s* group;
    struct pubsub_listener_s* listener_list_head;
    struct pubsub_topic_s* next;
};

struct pubsub_topic_group_s {
    struct fifoallocator_instance_s allocator;
    mutex_t topic_group_mtx;
};

// - Creates a new topic group with a separate memory pool. This new topic group is insulated from problems on other topic
//   groups such as the default topic group, e.g. badly-behaved listeners locking the allocator and blocking for too long,
//   or publishers publishing messages that are too large, causing the allocator to deallocate every other message.
void pubsub_create_topic_group(struct pubsub_topic_group_s* topic_group, size_t fifo_memory_pool_size, void* fifo_memory_pool);

// - Initializes a new topic object.
// - name may be NULL
// - topic_group may be NULL, in which case the default topic group is used.
void pubsub_init_topic(struct pubsub_topic_s* topic, const char* name, struct pubsub_topic_group_s* topic_group);

// - Initializes a listener object owned by the current thread and registers it on a topic.
void pubsub_init_and_register_listener(struct pubsub_topic_s* topic, struct pubsub_listener_s* listener);

// - Unregisters a listener from its topic.
void pubsub_listener_unregister(struct pubsub_listener_s* listener);

// - Resets a listener to a state of no messages pending.
void pubsub_listener_reset(struct pubsub_listener_s* listener);

// - Allocates a message on topic topic of size size, calls writer_cb(size, msg, ctx) to populate it, and publishes it.
void pubsub_publish_message(const struct pubsub_topic_s* topic, size_t size, pubsub_message_writer_func_ptr writer_cb, void* ctx);

// - Waits for up to timeout systicks for a new message to become available to listener.
// - timeout can be TIME_IMMEDIATE to check for an immediately available message, or TIME_INFINITE to wait forever for a message.
// - Returns true if a new message is available, false otherwise.
bool pubsub_listener_wait_timeout(const struct pubsub_listener_s* listener, systime_t timeout);

// - Waits for messages to become available to listener, calls handler_cb(msg_size, msg, ctx) to handle them. Returns after timeout has elapsed.
// - timeout can be TIME_IMMEDIATE to handle all immediately available messages, or TIME_INFINITE to handle messages forever.
// - Guarantees chronological handling of messages.
void pubsub_listener_handle_until_timeout(const struct pubsub_listener_s* listener, pubsub_message_handler_func_ptr handler_cb, void* ctx, systime_t timeout);

// - Waits for up to timeout systicks for a new message to become available to any listener in the listeners array.
// - timeout can be TIME_IMMEDIATE to check for an immediately available message, or TIME_INFINITE to wait forever for a message.
// - Returns true if a new message became available, false otherwise
bool pubsub_multiple_listener_wait_timeout(size_t num_listeners, const struct pubsub_listener_s** listeners, systime_t timeout);

// - Waits for messages to become avilable to any listener in the listeners array, calls handler_cbs[listener_index](msg_size, msg, ctx). Returns after timeout has elapsed.
// - timeout can be TIME_IMMEDIATE to handle all immediately available messages, or TIME_INFINITE to handle messages forever.
// - Guarantees chronological handling of messages within the same topic. Does not guarantee chronological handling of messages across the topic list.
void pubsub_multiple_listener_handle_until_timeout(size_t num_listeners, const struct pubsub_listener_s** listeners, pubsub_message_handler_func_ptr* handler_cbs, void* ctx, systime_t timeout);

#include <common/pubsub.h>
#include <common/helpers.h>
#include <common/ctor.h>

#ifndef OMD_PUBSUB_DEFAULT_ALLOCATOR_POOL_SIZE
#define OMD_PUBSUB_DEFAULT_ALLOCATOR_POOL_SIZE 512
#endif

OMD_PUBSUB_DECLARE_TOPIC_GROUP_STATIC(default_topic_group, OMD_PUBSUB_DEFAULT_ALLOCATOR_POOL_SIZE)

static void pubsub_delete_handler(void* delete_block);

void pubsub_create_topic_group(struct pubsub_topic_group_s* topic_group, size_t memory_pool_size, void* memory_pool) {
    if (!topic_group || !memory_pool) {
        return;
    }

    fifoallocator_init(&topic_group->allocator, memory_pool_size, memory_pool, pubsub_delete_handler);
    chMtxObjectInit(&topic_group->mtx);
}

void pubsub_init_topic(struct pubsub_topic_s* topic, struct pubsub_topic_group_s* topic_group) {
    if (!topic) {
        return;
    }

    if (!topic_group) {
        topic_group = &default_topic_group;
    }

    topic->message_list_tail = NULL;
    topic->group = topic_group;
    topic->listener_list_head = NULL;
}

void pubsub_init_and_register_listener(struct pubsub_topic_s* topic, struct pubsub_listener_s* listener) {
    if (!topic || !topic->group || !listener) {
        return;
    }

    // initialize listener
    listener->topic = topic;
    listener->next_message = NULL;
    listener->waiting_thread_reference_ptr = NULL;
    listener->handler_cb = NULL;
    listener->handler_cb_ctx = NULL;
    chMtxObjectInit(&listener->mtx);
    listener->next = NULL;

    // lock topic group
    chMtxLock(&topic->group->mtx);

    // append listener to topic's listener list
    LINKED_LIST_APPEND(struct pubsub_listener_s, topic->listener_list_head, listener);

    // unlock topic group
    chMtxUnlock(&topic->group->mtx);
}

void pubsub_listener_unregister(struct pubsub_listener_s* listener) {
    if (!listener) {
        return;
    }

    // lock topic group
    chMtxLock(&listener->topic->group->mtx);

    // remove listener from topic's listener list
    struct pubsub_listener_s** listener_list_next_ptr = &listener->topic->listener_list_head;
    while (*listener_list_next_ptr && *listener_list_next_ptr != listener) {
        listener_list_next_ptr = &(*listener_list_next_ptr)->next;
    }

    if (*listener_list_next_ptr) {
        *listener_list_next_ptr = listener->next;
    }

    chMtxUnlock(&listener->topic->group->mtx);
}

void pubsub_listener_reset(struct pubsub_listener_s* listener) {
    if (!listener) {
        return;
    }

    chMtxLock(&listener->mtx);
    listener->next_message = NULL;
    chMtxUnlock(&listener->mtx);
}

void pubsub_publish_message(struct pubsub_topic_s* topic, size_t size, pubsub_message_writer_func_ptr writer_cb, void* ctx) {
    if (!topic || !topic->group || !topic->listener_list_head) {
        return;
    }

    // lock topic group
    chMtxLock(&topic->group->mtx);

    struct pubsub_message_s* message = fifoallocator_allocate(&topic->group->allocator, size+sizeof(struct pubsub_message_s));

    if (!message) {
        chMtxUnlock(&topic->group->mtx);
        return;
    }

    message->topic = topic;
    message->next_in_topic = NULL;

    if (topic->message_list_tail) {
        topic->message_list_tail->next_in_topic = message;
    }
    topic->message_list_tail = message;

    if (writer_cb) {
        writer_cb(size, message->data, ctx);
    }

    struct pubsub_listener_s* listener = topic->listener_list_head;
    while (listener) {
        if (!listener->next_message) {
            // Lock to ensure atomic write of next_message
            chMtxLock(&listener->mtx);
            listener->next_message = message;
            chMtxUnlock(&listener->mtx);
        }

        chSysLock();
        if (listener->waiting_thread_reference_ptr) {
            chThdResumeI(listener->waiting_thread_reference_ptr, (msg_t)listener);
        }
        chSysUnlock();

        listener = listener->next;
    }

    chMtxUnlock(&topic->group->mtx);
}

void pubsub_listener_set_handler_cb(struct pubsub_listener_s* listener, pubsub_message_handler_func_ptr handler_cb, void* handler_cb_ctx) {
    if (!listener) {
        return;
    }

    chMtxLock(&listener->mtx);
    listener->handler_cb = handler_cb;
    listener->handler_cb_ctx = handler_cb_ctx;
    chMtxUnlock(&listener->mtx);
}

void pubsub_listener_handle_until_timeout(struct pubsub_listener_s* listener, systime_t timeout) {
    pubsub_multiple_listener_handle_until_timeout(1, &listener, timeout);
}

static struct pubsub_listener_s* pubsub_multiple_listener_wait_timeout_S(size_t num_listeners, struct pubsub_listener_s** listeners, systime_t timeout);

void pubsub_multiple_listener_handle_until_timeout(size_t num_listeners, struct pubsub_listener_s** listeners, systime_t timeout) {
    systime_t start = chVTGetSystemTimeX();
    systime_t elapsed = 0;

    do {
        syssts_t sts = chSysGetStatusAndLockX();
        struct pubsub_listener_s* listener_with_message = pubsub_multiple_listener_wait_timeout_S(num_listeners, listeners, timeout-elapsed);

        if (listener_with_message) {
            chMtxLockS(&listener_with_message->mtx);
            chSysRestoreStatusX(sts);

            struct pubsub_message_s* message = listener_with_message->next_message;

            if (listener_with_message->handler_cb) {
                size_t message_size = fifoallocator_get_block_size(message)-sizeof(struct pubsub_message_s);

                listener_with_message->handler_cb(message_size, message, listener_with_message->handler_cb_ctx);
            }

            listener_with_message->next_message = message->next_in_topic;

            chMtxUnlock(&listener_with_message->mtx);
        } else {
            chSysRestoreStatusX(sts);
        }

        if (timeout != TIME_INFINITE) {
            elapsed = chVTTimeElapsedSinceX(start);
        }
    } while(elapsed < timeout);
}

static struct pubsub_listener_s* pubsub_multiple_listener_wait_timeout_S(size_t num_listeners, struct pubsub_listener_s** listeners, systime_t timeout) {
    // Check for immediately available messages
    for (size_t i=0; i<num_listeners; i++) {
        if (listeners && listeners[i] && listeners[i]->next_message) {
            return listeners[i];
        }
    }

    // Point listeners' waiting thread references to our thread
    thread_reference_t trp = NULL;
    for (size_t i=0; i<num_listeners; i++) {
        if (listeners && listeners[i]) {
            listeners[i]->waiting_thread_reference_ptr = &trp;
        }
    }

    // Wait for a listener to wake us up
    msg_t message = chThdSuspendTimeoutS(&trp, timeout);

    struct pubsub_listener_s* ret = NULL;
    if (message != MSG_TIMEOUT) {
        ret = (struct pubsub_listener_s*)message;
    }

    // Set listeners' waiting thread references back to NULL
    for (size_t i=0; i<num_listeners; i++) {
        if (listeners && listeners[i]) {
            listeners[i]->waiting_thread_reference_ptr = NULL;
        }
    }

    return ret;
}

static void pubsub_delete_handler(void* delete_block) {
    // NOTE: this will be called during publishing in the publishing thread's context. The topic group will already be locked.
    struct pubsub_message_s* message_to_delete = delete_block;

    struct pubsub_listener_s* listener = message_to_delete->topic->listener_list_head;
    while (listener) {
        chSysLock();
        if (listener->next_message == message_to_delete) {
            chMtxLockS(&listener->mtx);
            listener->next_message = message_to_delete->next_in_topic;
            chMtxUnlockS(&listener->mtx);
        }
        chSysUnlock();
        listener = listener->next;
    }

    if (message_to_delete->topic->message_list_tail == message_to_delete) {
        message_to_delete->topic->message_list_tail = NULL;
    }
}

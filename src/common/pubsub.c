#include <common/pubsub.h>
#include <common/helpers.h>

#ifndef OMD_PUBSUB_DEFAULT_ALLOCATOR_POOL_SIZE
#define OMD_PUBSUB_DEFAULT_ALLOCATOR_POOL_SIZE 1024
#endif

MEMORYPOOL_DECL(listener_list_pool, sizeof(struct pubsub_listener_s), chCoreAllocAligned);

static struct pubsub_topic_s* topic_list_head;

struct pubsub_listener_s* pubsub_listen_to_topic(uint64_t key, struct msgfifo_instance_s* msgfifo, eventid_t event_id) {
    if (!msgfifo) {
        return NULL;
    }

    struct pubsub_topic_s* topic = topic_list_head;
    while (topic) {
        if (topic->key == key) {
            break;
        }

        topic = topic->next;
    }

    if (!topic) {
        return NULL;
    }

    struct pubsub_listener_s** listener_list_tail_next_ptr = &topic->listener_list_head;
    while (*listener_list_tail_next_ptr != NULL) {
        listener_list_tail_next_ptr = &(*listener_list_tail_next_ptr)->next;
    }

    struct pubsub_listener_s* new_listener = chPoolAlloc(&listener_list_pool);

    if (!new_listener) {
        return NULL;
    }

    chMBObjectInit(&new_listener->mailbox, new_listener->mailbox_buf, sizeof(new_listener->mailbox_buf)/sizeof(new_listener->mailbox_buf[0]));
    new_listener->msgfifo = msgfifo;
    new_listener->topic = topic;
    chEvtGetAndClearEvents(EVENT_MASK(event_id));
    chEvtRegisterMask(&new_listener->event_listener, &topic->event_source, EVENT_MASK(event_id));
    new_listener->next = NULL;

    *listener_list_tail_next_ptr = new_listener;

    return new_listener;
}

void pubsub_register_topic(uint64_t key, size_t size) {
    struct pubsub_topic_s* topic_list_tail = NULL;
    struct pubsub_topic_s* topic = topic_list_head;
    while (topic) {
        if (topic->key == key) {
            return;
        }
        topic_list_tail = topic;

        topic = topic->next;
    }

    struct pubsub_topic_s* new_topic = chCoreAllocAligned(sizeof(struct pubsub_topic_s), sizeof(void*));

    if (new_topic) {
        return;
    }

    new_topic->key = key;
    new_topic->size = size;
    chEvtObjectInit(&new_topic->event_source);
    new_topic->listener_list_head = NULL;

    new_topic->next = NULL;

    if (!topic_list_tail) {
        topic_list_head = new_topic;
    } else {
        topic_list_tail->next = new_topic;
    }
}

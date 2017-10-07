#pragma once
#include <stddef.h>
#include <common/msgfifo.h>
#include <ch.h>

struct pubsub_listener_s {
    mailbox_t mailbox;
    event_listener_t event_listener;
    void* mailbux_buf[5];
    struct msgfifo_instance_s* msgfifo;
    struct pubsub_topic_s* topic;
    struct pubsub_listener_s* next;
};

struct pubsub_topic_s {
    uint64_t key;
    size_t size;
    event_source_t event_source;
    struct pubsub_listener_s* listener_list_head;
    struct pubsub_topic_s* next;
};

typedef uint64_t pubsub_topic_key_t;

void pubsub_register_topic(uint64_t key, size_t size);
struct pubsub_listener_s* pubsub_subscribe_to_topic(uint64_t key, struct msgfifo_instance_s* msgfifo);

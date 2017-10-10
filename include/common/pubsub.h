#pragma once
#include <stddef.h>
#include <common/fifoallocator.h>
#include <ch.h>

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
    mutex_t mtx;
    event_listener_t event_listener;
    struct pubsub_listener_s* next;
};

struct pubsub_topic_s {
    uint64_t key;
    size_t size;
    struct pubsub_message_s* message_list_tail;
    struct pubsub_listener_s* listener_list_head;
    event_source_t event_source;
    struct fifoallocator_instance_s* allocator;
    struct pubsub_topic_s* next;
};

void pubsub_register_topic(uint64_t key, size_t size);
struct pubsub_listener_s* pubsub_subscribe_to_topic(uint64_t key, struct msgfifo_instance_s* msgfifo);

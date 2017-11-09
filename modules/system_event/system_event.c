#include "system_event.h"
#include <common/ctor.h>
#include <pubsub/pubsub.h>

struct pubsub_topic_s system_event_topic;

RUN_ON(PUBSUB_TOPIC_INIT) {
    pubsub_init_topic(&system_event_topic, NULL);
}

void system_event_publish(enum system_event_t event) {
    pubsub_publish_message(&system_event_topic, sizeof(event), pubsub_copy_writer_func, &event);
}

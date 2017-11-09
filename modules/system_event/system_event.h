#pragma once

enum system_event_t {
    SYSTEM_EVENT_REBOOT_IMMINENT,
};

extern struct pubsub_topic_s system_event_topic;

void system_event_publish(enum system_event_t event);

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <modules/pubsub/pubsub.h>

enum pin_change_type_t {
    PIN_CHANGE_TYPE_LOW_LEVEL = 0,
    PIN_CHANGE_TYPE_FALLING,
    PIN_CHANGE_TYPE_RISING,
    PIN_CHANGE_TYPE_BOTH
};


struct pin_change_msg_s {
    systime_t timestamp;
};

bool pin_change_publisher_enable_pin(uint32_t line, enum pin_change_type_t mode, struct pubsub_topic_s* topic);
bool pin_change_publisher_enable_pin_oneshot(uint32_t line, enum pin_change_type_t mode, struct pubsub_topic_s* topic); // Masks the interrupt until a subscriber re-enables it.
void pin_change_publisher_disable_pin(uint32_t line);
void pin_change_publisher_unmask_pin(uint32_t line);

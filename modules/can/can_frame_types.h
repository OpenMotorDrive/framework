#pragma once

#include <ch.h>
#include <stdbool.h>
#include <stdint.h>
#include <modules/pubsub/pubsub.h>

typedef uint32_t can_frame_priority_t;

struct can_frame_s {
    uint8_t RTR:1;
    uint8_t IDE:1;
    uint8_t DLC:4;
    union {
        uint32_t SID:11;
        uint32_t EID:29;
    };
    union {
        uint8_t data[8];
        uint32_t data32[2];
    };
};

struct can_rx_frame_s {
    struct can_frame_s content;
    systime_t rx_systime;
};

struct can_tx_frame_s {
    struct can_frame_s content;
    systime_t creation_systime;
    systime_t tx_timeout;
    struct pubsub_topic_s* completion_topic;
    struct can_tx_frame_s* next;
};

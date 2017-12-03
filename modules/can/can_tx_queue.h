#pragma once

#include "can_frame_types.h"

struct can_tx_queue_s {
    struct can_tx_frame_s* head;
};

bool can_tx_queue_init(struct can_tx_queue_s* instance);

void can_tx_queue_push_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);
void can_tx_queue_push(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);

void can_tx_queue_push_ahead_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);
void can_tx_queue_push_ahead(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);

bool can_tx_queue_iterate_I(struct can_tx_queue_s* instance, struct can_tx_frame_s** frame);

struct can_tx_frame_s* can_tx_queue_peek_I(struct can_tx_queue_s* instance);
struct can_tx_frame_s* can_tx_queue_peek(struct can_tx_queue_s* instance);

void can_tx_queue_pop_I(struct can_tx_queue_s* instance);
void can_tx_queue_pop(struct can_tx_queue_s* instance);

struct can_tx_frame_s* can_tx_queue_pop_expired_I(struct can_tx_queue_s* instance);
struct can_tx_frame_s* can_tx_queue_pop_expired(struct can_tx_queue_s* instance);

void can_tx_queue_remove_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);

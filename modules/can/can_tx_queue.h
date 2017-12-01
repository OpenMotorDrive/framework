#pragma once

#include "can_frame_types.h"

struct can_tx_queue_s {
    memory_pool_t frame_pool;
    struct can_tx_frame_s* head;
    struct can_tx_frame_s* stage_head;
};

bool can_tx_queue_init(struct can_tx_queue_s* instance, size_t queue_max_len);

struct can_tx_frame_s* can_tx_queue_allocate_I(struct can_tx_queue_s* instance);
void can_tx_queue_free_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* free_frame);
void can_tx_queue_free(struct can_tx_queue_s* instance, struct can_tx_frame_s* free_frame);

void can_tx_queue_stage_push_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);

void can_tx_queue_push_ahead_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);
void can_tx_queue_push_ahead(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);

bool can_tx_queue_iterate_I(struct can_tx_queue_s* instance, struct can_tx_frame_s** frame);

bool can_tx_stage_iterate_I(struct can_tx_queue_s* instance, struct can_tx_frame_s** frame);

void can_tx_queue_remove_I(struct can_tx_queue_s* instance, struct can_tx_frame_s* frame);

struct can_tx_frame_s* can_tx_queue_pop_I(struct can_tx_queue_s* instance);
struct can_tx_frame_s* can_tx_queue_pop(struct can_tx_queue_s* instance);

struct can_tx_frame_s* can_tx_queue_peek_I(struct can_tx_queue_s* instance);
struct can_tx_frame_s* can_tx_queue_peek(struct can_tx_queue_s* instance);

void can_tx_queue_commit_staged_pushes_I(struct can_tx_queue_s* instance);
void can_tx_queue_free_staged_pushes_I(struct can_tx_queue_s* instance);

#pragma once
#include "can_frame_types.h"
#include <stdbool.h>
#include <stdint.h>

bool can_tx_frame_expired_X(struct can_tx_frame_s* frame);
systime_t can_tx_frame_time_until_expire_X(struct can_tx_frame_s* frame, systime_t t_now);
can_frame_priority_t can_get_frame_priority_X(const struct can_frame_s* frame);
can_frame_priority_t can_get_tx_frame_priority_X(const struct can_tx_frame_s* frame);

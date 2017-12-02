#pragma once

#include "can_frame_types.h"
#include <modules/pubsub/pubsub.h>
#include <stdbool.h>
#include <stdint.h>
#include <ch.h>

struct can_instance_s;

struct can_transmit_completion_msg_s {
    systime_t completion_systime;
    bool transmit_success;
};

struct can_instance_s* can_get_instance(uint8_t can_idx);

bool can_iterate_instances(struct can_instance_s** instance_ptr);

void can_start_I(struct can_instance_s* instance, bool silent, bool auto_retransmit, uint32_t baudrate);
void can_start(struct can_instance_s* instance, bool silent, bool auto_retransmit, uint32_t baudrate);

void can_stop_I(struct can_instance_s* instance);
void can_stop(struct can_instance_s* instance);

struct pubsub_topic_s* can_get_rx_topic(struct can_instance_s* instance);

void can_set_silent_mode(struct can_instance_s* instance, bool silent);
void can_set_auto_retransmit_mode(struct can_instance_s* instance, bool auto_retransmit);
void can_set_baudrate(struct can_instance_s* instance, uint32_t baudrate);
uint32_t can_get_baudrate(struct can_instance_s* instance);
bool can_get_baudrate_confirmed(struct can_instance_s* instance);

struct can_tx_frame_s* can_allocate_frame_I(struct can_instance_s* instance);
void can_stage_frame_I(struct can_instance_s* instance, struct can_tx_frame_s* frame);
void can_send_staged_frames_I(struct can_instance_s* instance, systime_t tx_timeout, struct pubsub_topic_s* completion_topic);
void can_free_staged_frames_I(struct can_instance_s* instance);

bool can_send_I(struct can_instance_s* instance, struct can_frame_s* frame, systime_t tx_timeout, struct pubsub_topic_s* completion_topic);
bool can_send(struct can_instance_s* instance, struct can_frame_s* frame, systime_t tx_timeout, struct pubsub_topic_s* completion_topic);

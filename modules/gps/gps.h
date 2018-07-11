#pragma once

#include <ch.h>
#include <hal.h>
#include <modules/pubsub/pubsub.h>

#ifndef PARSER_BUFFER_SIZE
#define PARSER_BUFFER_SIZE 1024
#endif

enum message_validity_t {
    MESSAGE_EMPTY,
    MESSAGE_INVALID,
    MESSAGE_INCOMPLETE,
    MESSAGE_VALID
};

struct gps_msg {
    struct gps_handle_s *gps_handle;
    uint8_t class_id;
    uint8_t msg_id;
    size_t msg_len;
    void *frame_buffer;
};

struct gps_handle_s {
    enum message_validity_t state;
    uint8_t parser_buffer[PARSER_BUFFER_SIZE];
    uint8_t parser_cnt;
};

bool gps_ubx_init_msg_topic(struct gps_handle_s* gps_handle, uint8_t class_id, uint8_t msg_id, void* frame_buffer, size_t frame_buffer_len, struct pubsub_topic_s* topic);
bool gps_init(struct gps_handle_s* gps_handle);
bool gps_spin(struct gps_handle_s* gps_handle, uint8_t byte);

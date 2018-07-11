#include "gps.h"

#include <modules/worker_thread/worker_thread.h>
#include <common/ctor.h>
#include <common/helpers.h>
#include <modules/worker_thread/worker_thread.h>
#include <string.h>
#include <modules/uavcan_debug/uavcan_debug.h>

#ifndef GPS_MSG_QUEUE_DEPTH
#define GPS_MSG_QUEUE_DEPTH 4
#endif

struct gps_msg_publisher_topic_s {
    uint8_t class_id;
    uint8_t msg_id;
    void* frame_buffer;
    struct gps_handle_s *gps_handle;
    size_t frame_buffer_len;
    struct pubsub_topic_s* topic;
    struct gps_msg_publisher_topic_s* next;
};


static struct gps_msg_publisher_topic_s *gps_msg_topic_list_head;
MEMORYPOOL_DECL(gps_msg_publisher_topic_list_pool, sizeof(struct gps_msg_publisher_topic_s), chCoreAllocAlignedI);

bool gps_ubx_init_msg_topic(struct gps_handle_s* gps_handle, uint8_t class_id, uint8_t msg_id, void* frame_buffer, size_t frame_buffer_len, struct pubsub_topic_s* topic)
{
    chSysLock();
    if (!topic || !frame_buffer) {
        return false;
    }
    struct gps_msg_publisher_topic_s *topic_handle = chPoolAllocI(&gps_msg_publisher_topic_list_pool);
    if (!topic_handle) {
        uavcan_send_debug_msg(LOG_LEVEL_ERROR, "GPS", "Failed to init topic for 0x%x 0x%x", class_id, msg_id);
        return false;
    }
    topic_handle->class_id = class_id;
    topic_handle->msg_id = msg_id;
    topic_handle->gps_handle = gps_handle;
    topic_handle->frame_buffer = frame_buffer;
    topic_handle->frame_buffer_len = frame_buffer_len;
    topic_handle->topic = topic;
    LINKED_LIST_APPEND(struct gps_msg_publisher_topic_s, gps_msg_topic_list_head, topic_handle);
    chSysUnlock();
    return true;
}

bool gps_init(struct gps_handle_s* gps_handle)
{
    if (gps_handle == NULL) {
        return false;
    }
    gps_handle->state = MESSAGE_EMPTY;
    memset(gps_handle->parser_buffer, 0, sizeof(struct gps_handle_s));
    gps_handle->parser_cnt = 0;
    return true;
}

static void gps_ubx_frame_received(size_t length, uint8_t *buffer)
{
    struct gps_msg_publisher_topic_s* topic_handle = gps_msg_topic_list_head;
    while(topic_handle) {
        if (topic_handle->class_id == buffer[2] && topic_handle->msg_id == buffer[3]) {
            //publish gps message
            struct gps_msg msg;
            if (topic_handle->frame_buffer_len >= length - 8) {
                memcpy(topic_handle->frame_buffer, buffer+6, length - 8);
            }
            msg.class_id = buffer[2];
            msg.msg_id = buffer[3];
            msg.msg_len = length - 8;
            msg.frame_buffer = topic_handle->frame_buffer;
            pubsub_publish_message(topic_handle->topic, sizeof(struct gps_msg), pubsub_copy_writer_func, &msg);
        }
        topic_handle = topic_handle->next;
    }
}

static enum message_validity_t check_ubx_message(struct gps_handle_s* gps_handle, size_t* msg_len) {
    if (gps_handle->parser_cnt == 0) {
        return MESSAGE_EMPTY;
    }
    if (gps_handle->parser_cnt >= 1 && gps_handle->parser_buffer[0] != 0xB5) {
        return MESSAGE_INVALID;
    }
    if (gps_handle->parser_cnt >= 2 && gps_handle->parser_buffer[1] != 0x62) {
        return MESSAGE_INVALID;
    }
    if (gps_handle->parser_cnt < 8) {
        return MESSAGE_INCOMPLETE;
    }
    size_t len_field_val = gps_handle->parser_buffer[4] | (uint16_t)gps_handle->parser_buffer[5]<<8;
    if (len_field_val+8 > PARSER_BUFFER_SIZE) {
        return MESSAGE_INVALID;
    }

    if (gps_handle->parser_cnt < len_field_val+8) {
        return MESSAGE_INCOMPLETE;
    }

    uint8_t ck_a_provided = gps_handle->parser_buffer[6+len_field_val];
    uint8_t ck_b_provided = gps_handle->parser_buffer[7+len_field_val];

    uint8_t ck_a_computed = 0;
    uint8_t ck_b_computed = 0;

    for(uint16_t i=2; i < (len_field_val+6); i++) {
        ck_a_computed += gps_handle->parser_buffer[i];
        ck_b_computed += ck_a_computed;
    }

    if (ck_a_provided == ck_a_computed && ck_b_provided == ck_b_computed) {
        *msg_len = len_field_val+8;
        return MESSAGE_VALID;
    } else {
        return MESSAGE_INVALID;
    }
}


bool gps_spin(struct gps_handle_s* gps_handle, uint8_t byte)
{
    // Append byte to buffer
    gps_handle->parser_buffer[gps_handle->parser_cnt++] = byte;
    size_t msg_len;
    gps_handle->state = check_ubx_message(gps_handle, &msg_len);

    while (gps_handle->state == MESSAGE_INVALID) {
        gps_handle->parser_cnt--;
        memmove(gps_handle->parser_buffer, gps_handle->parser_buffer+1, gps_handle->parser_cnt);
        gps_handle->state = check_ubx_message(gps_handle, &msg_len);
    }

    if (gps_handle->state == MESSAGE_VALID) {
        gps_ubx_frame_received(gps_handle->parser_cnt, gps_handle->parser_buffer);
        gps_handle->parser_cnt = 0;
        return true;
    }
    return false;
}

#pragma once
#include "can_frame_types.h"

typedef void (*driver_start_t)(void* ctx, bool silent, bool auto_retransmit, uint32_t baudrate);
typedef void (*driver_stop_t)(void* ctx);

typedef bool (*driver_mailbox_abort_t)(void* ctx, uint8_t mb_idx);
typedef bool (*driver_load_tx_mailbox_t)(void* ctx, uint8_t mb_idx, struct can_frame_s* frame);
typedef bool (*driver_pop_rx_frame_t)(void* ctx, uint8_t mb_idx, struct can_frame_s* frame);
typedef bool (*driver_rx_frame_available_t)(void* ctx, uint8_t mb_idx);

struct can_driver_iface_s {
    driver_start_t start;
    driver_stop_t stop;
    driver_mailbox_abort_t abort_tx_mailbox_I;
    driver_load_tx_mailbox_t load_tx_mailbox_I;
};

struct can_instance_s* can_driver_register(uint8_t can_idx, void* driver_ctx, const struct can_driver_iface_s* driver_iface, uint8_t num_tx_mailboxes, uint8_t num_rx_mailboxes, uint8_t rx_fifo_depth);
void can_driver_tx_request_complete_I(struct can_instance_s* instance, uint8_t mb_idx, bool transmit_success, systime_t completion_systime);
void can_driver_rx_frame_received_I(struct can_instance_s* instance, uint8_t mb_idx, systime_t rx_systime, struct can_frame_s* frame);

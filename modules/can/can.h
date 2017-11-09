#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <hal.h>

void can_init(uint8_t can_idx, uint32_t baud, bool silent);
uint32_t can_get_baudrate(uint8_t can_idx);
bool can_get_baudrate_confirmed(uint8_t can_idx);
msg_t can_receive_timeout(uint8_t can_idx, canmbx_t mailbox, CANRxFrame* crfp, systime_t timeout);
msg_t can_transmit_timeout(uint8_t can_idx, canmbx_t mailbox, const CANTxFrame* ctfp, systime_t timeout);

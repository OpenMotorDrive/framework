/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/can.h>
#include <common/timing.h>
#include <hal.h>
#include <string.h>

static const uint32_t valid_baudrates[] = {
    125000,
    250000,
    500000,
    1000000
};

#define NUM_VALID_BAUDRATES (sizeof(valid_baudrates)/sizeof(valid_baudrates[0]))

static uint32_t baudrate = 0;
static bool successful_recv = false;

void canbus_init(uint32_t baud, bool silent) {
    if (!canbus_baudrate_valid(baud)) {
        return;
    }

    baudrate = baud;
    successful_recv = false;

    const CANConfig cancfg = {
        CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
        (silent?CAN_BTR_SILM:0) | CAN_BTR_SJW(0) | CAN_BTR_TS2(2-1) |
        CAN_BTR_TS1(15-1) | CAN_BTR_BRP((STM32_PCLK1/18)/baudrate - 1)
    };

    canStart(&CAND1, &cancfg);
}

uint32_t canbus_get_confirmed_baudrate(void) {
    if (successful_recv && canbus_baudrate_valid(baudrate)) {
        return baudrate;
    }
    return 0;
}

void canbus_autobaud_start(struct canbus_autobaud_state_s* state, uint32_t initial_baud, uint32_t switch_interval_us) {
    uint32_t tnow_us = micros();
    state->start_us = tnow_us;
    state->last_switch_us = tnow_us;
    state->switch_interval_us = switch_interval_us;


    for (uint8_t i=0; i<NUM_VALID_BAUDRATES; i++) {
        state->curr_baud_idx = i;
        if (valid_baudrates[i] == initial_baud) {
            break;
        }
    }
    state->success = false;

    canbus_init(valid_baudrates[state->curr_baud_idx], true);
}

bool canbus_baudrate_valid(uint32_t baud) {
    for (uint8_t i=0; i<NUM_VALID_BAUDRATES; i++) {
        if (valid_baudrates[i] == baud) {
            return true;
        }
    }
    return false;
}

uint32_t canbus_autobaud_update(struct canbus_autobaud_state_s* state) {
    if (state->success) {
        return valid_baudrates[state->curr_baud_idx];
    }

    uint32_t tnow_us = micros();
    uint32_t time_since_switch_us = tnow_us - state->last_switch_us;

    struct canbus_msg msg;
    if (canbus_recv_message(&msg)) {
        state->success = true;
        return valid_baudrates[state->curr_baud_idx];
    }

    if (time_since_switch_us >= state->switch_interval_us) {
        state->last_switch_us = tnow_us;
        if (state->curr_baud_idx == 0) {
            state->curr_baud_idx = NUM_VALID_BAUDRATES-1;
        } else {
            state->curr_baud_idx--;
        }
        canbus_init(valid_baudrates[state->curr_baud_idx], true);
    }
    return 0;
}

bool canbus_send_message(struct canbus_msg* msg) {
    if (!msg || msg->dlc > 8) {
        return false;
    }

    CANTxFrame txmsg;
    if (msg->ide) {
        txmsg.EID = msg->id;
    } else {
        txmsg.SID = msg->id;
    }

    txmsg.IDE = msg->ide;
    txmsg.RTR = msg->rtr;
    txmsg.DLC = msg->dlc;
    memcpy(txmsg.data8, msg->data, msg->dlc);

    if (canTransmitTimeout(&CAND1, CAN_ANY_MAILBOX, &txmsg, TIME_IMMEDIATE) == MSG_OK) {
        return true;
    } else {
        return false;
    }
}

bool canbus_recv_message(struct canbus_msg* msg) {
    if (!msg) {
        return false;
    }

    CANRxFrame rxmsg;
    if (canReceiveTimeout(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
        if (rxmsg.IDE) {
            msg->id = rxmsg.EID;
        } else {
            msg->id = rxmsg.SID;
        }

        msg->ide = rxmsg.IDE;
        msg->rtr = rxmsg.RTR;
        msg->dlc = rxmsg.DLC;
        memcpy(msg->data, rxmsg.data8, rxmsg.DLC);

        return true;
    } else {
        return false;
    }
}

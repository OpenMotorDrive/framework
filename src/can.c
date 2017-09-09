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

    // canStart ...
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
    // canSend
}

bool canbus_recv_message(struct canbus_msg* msg) {
    // canReceive
}

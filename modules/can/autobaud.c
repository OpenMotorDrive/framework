#include "can.h"
#include "autobaud.h"
#include <timing/timing.h>

static const uint32_t valid_baudrates[] = {
    125000,
    250000,
    500000,
    1000000
};

#define NUM_VALID_BAUDRATES (sizeof(valid_baudrates)/sizeof(valid_baudrates[0]))

void can_autobaud_start(struct can_autobaud_state_s* state, uint8_t can_idx, uint32_t initial_baud, uint32_t switch_interval_us) {
    uint32_t tnow_us = micros();
    state->can_idx = can_idx;
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

    can_init(state->can_idx, valid_baudrates[state->curr_baud_idx], true);
}

bool can_autobaud_baudrate_valid(uint32_t baud) {
    for (uint8_t i=0; i<NUM_VALID_BAUDRATES; i++) {
        if (valid_baudrates[i] == baud) {
            return true;
        }
    }
    return false;
}

uint32_t can_autobaud_update(struct can_autobaud_state_s* state) {
    if (state->success) {
        return valid_baudrates[state->curr_baud_idx];
    }

    uint32_t tnow_us = micros();
    uint32_t time_since_switch_us = tnow_us - state->last_switch_us;

    CANRxFrame rxmsg;
    can_receive_timeout(state->can_idx, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE);
    if (can_get_baudrate_confirmed(state->can_idx)) {
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
        can_init(state->can_idx, valid_baudrates[state->curr_baud_idx], true);
    }
    return 0;
}

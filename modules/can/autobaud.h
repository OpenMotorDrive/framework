#pragma once

#include <stdint.h>
#include <stdbool.h>

struct can_autobaud_state_s {
    uint8_t can_idx;
    bool success;
    uint32_t start_us;
    uint32_t last_switch_us;
    uint32_t switch_interval_us;
    uint8_t curr_baud_idx;
};

void can_autobaud_start(struct can_autobaud_state_s* state, uint8_t can_idx, uint32_t initial_baud, uint32_t switch_interval_us);
uint32_t can_autobaud_update(struct can_autobaud_state_s* state);
bool can_autobaud_baudrate_valid(uint32_t baud);

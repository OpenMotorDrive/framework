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

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct canbus_autobaud_state_s {
    bool success;
    uint32_t start_us;
    uint32_t last_switch_us;
    uint32_t switch_interval_us;
    uint8_t curr_baud_idx;
};

struct canbus_msg {
    uint32_t id;
    bool ide;
    bool rtr;
    uint8_t dlc;
    uint8_t data[8];
};

void canbus_autobaud_start(struct canbus_autobaud_state_s* state, uint32_t initial_baud, uint32_t switch_interval_us);
uint32_t canbus_autobaud_update(struct canbus_autobaud_state_s* state);

bool canbus_baudrate_valid(uint32_t baud);
uint32_t canbus_get_confirmed_baudrate(void);
void canbus_init(uint32_t baud, bool silent);
bool canbus_send_message(struct canbus_msg* msg);
bool canbus_recv_message(struct canbus_msg* msg);

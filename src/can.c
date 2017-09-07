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

    // Enable peripheral clock
    rcc_periph_clock_enable(RCC_CAN);

    rcc_periph_clock_enable(BOARD_CONFIG_CAN_RX_GPIO_PORT_RCC);
    gpio_mode_setup(BOARD_CONFIG_CAN_RX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BOARD_CONFIG_CAN_RX_GPIO_PIN);
    gpio_set_af(BOARD_CONFIG_CAN_RX_GPIO_PORT, BOARD_CONFIG_CAN_RX_GPIO_ALTERNATE_FUNCTION, BOARD_CONFIG_CAN_RX_GPIO_PIN);


    rcc_periph_clock_enable(BOARD_CONFIG_CAN_TX_GPIO_PORT_RCC);
    gpio_mode_setup(BOARD_CONFIG_CAN_TX_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BOARD_CONFIG_CAN_TX_GPIO_PIN);
    gpio_set_af(BOARD_CONFIG_CAN_TX_GPIO_PORT, BOARD_CONFIG_CAN_TX_GPIO_ALTERNATE_FUNCTION, BOARD_CONFIG_CAN_TX_GPIO_PIN);

    uint8_t brp = 2000000/baudrate;

    can_reset(CAN1);
    can_init(
        CAN1,             /* CAN register base address */
        false,            /* TTCM: Time triggered comm mode? */
        true,             /* ABOM: Automatic bus-off management? */
        false,            /* AWUM: Automatic wakeup mode? */
        false,            /* NART: No automatic retransmission? */
        false,            /* RFLM: Receive FIFO locked mode? */
        true,             /* TXFP: Transmit FIFO priority? */
        CAN_BTR_SJW_1TQ,  /* Resynchronization time quanta jump width.*/
        CAN_BTR_TS1_15TQ, /* Time segment 1 time quanta width. */
        CAN_BTR_TS2_2TQ,  /* Time segment 2 time quanta width. */
        brp,                /* Baud rate prescaler. */
        false,            /* Loopback */
        silent             /* Silent */
    );

    can_filter_id_mask_32bit_init(
        CAN1,  /* CAN register base address */
        0,     /* Filter ID */
        0,     /* CAN ID */
        0,     /* CAN ID mask */
        0,     /* FIFO assignment (here: FIFO0) */
        true
    );
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
    return can_transmit(
        CAN1,
        msg->id,  /* (EX/ST)ID: CAN ID */
        msg->ide, /* IDE: CAN ID extended? */
        msg->rtr, /* RTR: Request transmit? */
        msg->dlc, /* DLC: Data length */
        msg->data
    ) != -1;
}

bool canbus_recv_message(struct canbus_msg* msg) {
    if ((CAN_RF0R(CAN1)&0b11) == 0) {
        return false;
    }
    uint32_t fmi;
    can_receive(
        CAN1,
        0,
        true,
        &(msg->id),
        &(msg->ide),
        &(msg->rtr),
        &fmi,
        &(msg->dlc),
        msg->data);

    successful_recv = true;

    return true;
}

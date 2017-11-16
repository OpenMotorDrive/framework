#include "can.h"

#define NUM_CAN_INSTANCES 1

struct can_instance_s {
    uint32_t baudrate;
    bool baudrate_confirmed;
};

static struct can_instance_s can_instances[NUM_CAN_INSTANCES];

static CANDriver* can_get_device(uint8_t can_idx) {
    switch(can_idx) {
        case 0:
            return &CAND1;
    }
    return NULL;
}

void can_init(uint8_t can_idx, uint32_t baudrate, bool silent) {
    CANConfig cancfg = BOARD_CAN_CONFIG(baudrate, silent);

    canStart(can_get_device(can_idx), &cancfg);

    if (baudrate != can_instances[can_idx].baudrate) {
        can_instances[can_idx].baudrate_confirmed = false;
    }
}

bool can_get_baudrate_confirmed(uint8_t can_idx) {
    return can_instances[can_idx].baudrate_confirmed;
}

uint32_t can_get_baudrate(uint8_t can_idx) {
    return can_instances[can_idx].baudrate;
}

msg_t can_receive_timeout(uint8_t can_idx, canmbx_t mailbox, CANRxFrame* crfp, systime_t timeout) {
    msg_t ret = canReceiveTimeout(can_get_device(can_idx), mailbox, crfp, timeout);
    if (ret == MSG_OK) {
        can_instances[can_idx].baudrate_confirmed = true;
    }
    return ret;
}

msg_t can_transmit_timeout(uint8_t can_idx, canmbx_t mailbox, const CANTxFrame* ctfp, systime_t timeout) {
    return canTransmitTimeout(can_get_device(can_idx), mailbox, ctfp, timeout);
}

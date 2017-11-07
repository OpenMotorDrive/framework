#include "can.h"

CANDriver* can_get_device(uint8_t can_idx) {
    switch(can_idx) {
        case 0:
            return &CAND1;
    }
    return NULL;
}

void can_init(uint8_t can_idx, uint32_t baudrate, bool silent) {
    const CANConfig cancfg = {
        CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
        (silent?CAN_BTR_SILM:0) | CAN_BTR_SJW(0) | CAN_BTR_TS2(2-1) |
        CAN_BTR_TS1(15-1) | CAN_BTR_BRP((STM32_PCLK1/18)/baudrate - 1)
    };

    canStart(can_get_device(can_idx), &cancfg);
}

#include <hal.h>

void boardInit(void) {
    palSetLineMode(BOARD_PAL_LINE_CAN_RX, PAL_MODE_INPUT);
    palSetLineMode(BOARD_PAL_LINE_CAN_TX, PAL_MODE_OUTPUT_PUSHPULL);
}

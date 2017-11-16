#pragma once

#include <stdint.h>

#define STM32F302x8

#define BOARD_PARAM1_FLASH_SIZE ((size_t)&_param1_flash_sec_end - (size_t)&_param1_flash_sec)
#define BOARD_PARAM2_FLASH_SIZE ((size_t)&_param2_flash_sec_end - (size_t)&_param2_flash_sec)

#define BOARD_PARAM1_ADDR (&_param1_flash_sec)
#define BOARD_PARAM2_ADDR (&_param2_flash_sec)

extern uint8_t _param1_flash_sec;
extern uint8_t _param1_flash_sec_end;
extern uint8_t _param2_flash_sec;
extern uint8_t _param2_flash_sec_end;

#define BOARD_CAN_CONFIG(baudrate, silent) { \
CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP, \
(silent?CAN_BTR_SILM:0) | CAN_BTR_SJW(0) | CAN_BTR_TS2(2-1) | \
CAN_BTR_TS1(15-1) | CAN_BTR_BRP((STM32_PCLK1/18)/baudrate - 1) \
}

void board_get_unique_id(uint8_t* buf, uint8_t len);

#if !defined(_FROM_ASM_)
void boardInit(void);
#endif /* _FROM_ASM_ */

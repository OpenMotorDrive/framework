#pragma once

#include <stdint.h>
#include <modules/platform_stm32f302x8/platform_stm32f302x8.h>

#define BOARD_PAL_LINE_SPI3_SCK PAL_LINE(GPIOB,3)
#define BOARD_PAL_LINE_SPI3_MISO PAL_LINE(GPIOB,4)
#define BOARD_PAL_LINE_SPI3_MOSI PAL_LINE(GPIOB,5)
#define BOARD_PAL_LINE_SPI3_UWB_CS PAL_LINE(GPIOB,0) // NOTE: never drive high by external source
#define BOARD_PAL_LINE_UWB_NRST PAL_LINE(GPIOA,0)
#define BOARD_PAL_LINE_CAN_RX PAL_LINE(GPIOA,11)
#define BOARD_PAL_LINE_CAN_TX PAL_LINE(GPIOA,12)

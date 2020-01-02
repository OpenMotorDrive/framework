#pragma once

#include <stdint.h>
#include <modules/platform_stm32f401/platform_stm32f401.h>

/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0U
#endif

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                24000000U
#endif

#define STM32F401xC
/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD                   330U

#define BOARD_CONFIG_HW_NAME "org.skt.uwb"
#define BOARD_CONFIG_HW_MAJOR_VER 1
#define BOARD_CONFIG_HW_MINOR_VER 0

#define BOARD_CONFIG_HW_INFO_STRUCTURE { \
    .hw_name = BOARD_CONFIG_HW_NAME, \
    .hw_major_version = BOARD_CONFIG_HW_MAJOR_VER, \
    .hw_minor_version = BOARD_CONFIG_HW_MINOR_VER, \
    .board_desc_fmt = SHARED_HW_INFO_BOARD_DESC_FMT_NONE, \
    .board_desc = 0, \
}

#define BOARD_PAL_LINE_SPI_SCK PAL_LINE(GPIOB,3)
#define BOARD_PAL_LINE_SPI_MISO PAL_LINE(GPIOB,4)
#define BOARD_PAL_LINE_SPI_MOSI PAL_LINE(GPIOB,5)
#define BOARD_PAL_LINE_SPI_UWB_CS PAL_LINE(GPIOA,15) // NOTE: never drive high by external source
#define BOARD_PAL_LINE_UWB_NRST PAL_LINE(GPIOB,8)
#define BOARD_PAL_LINE_UWB_IRQ PAL_LINE(GPIOB,12)
#define BOARD_PAL_LINE_CAN_RX PAL_LINE(GPIOB,6)
#define BOARD_PAL_LINE_CAN_TX PAL_LINE(GPIOB,7)
#define DW1000_SPI_BUS 3

#define SERIAL_DEFAULT_BITRATE 57600

/*
 * IO pins assignments.
 */

#define LINE_UART2_RTS              PAL_LINE(GPIOA, 0U)
#define LINE_UART2_CTS              PAL_LINE(GPIOA, 1U)
#define LINE_UART2_RX               PAL_LINE(GPIOA, 2U)
#define LINE_UART2_TX               PAL_LINE(GPIOA, 3U)

#define LINE_OTG_HS_VBUS            PAL_LINE(GPIOA, 9U)
#define LINE_OTG_HS_DM              PAL_LINE(GPIOA, 11U)
#define LINE_OTG_HS_DP              PAL_LINE(GPIOA, 12U)

#define LINE_SWDIO                  PAL_LINE(GPIOA, 13U)
#define LINE_SWCLK                  PAL_LINE(GPIOA, 14U)

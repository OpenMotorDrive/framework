#pragma once

#include <stdint.h>
#include <modules/platform_stm32f103xB/platform_stm32f103xB.h>

#define BOARD_PAL_LINE_CAN_RX PAL_LINE(GPIOA,11)
#define BOARD_PAL_LINE_CAN_TX PAL_LINE(GPIOA,12)

// CAN I/O for maple-mini and mRo gps is on PortA
#define TXPORT GPIOA

// maple-mini
#define LEDPORT_MM GPIOB
#define LEDPIN_MM 1

// mRo GPS module
#define LEDPORT GPIOA
#define LEDPIN 4

#define CONFIG_LED palSetPadMode(LEDPORT, LEDPIN, PAL_MODE_OUTPUT_PUSHPULL); palSetPadMode(LEDPORT_MM, LEDPIN_MM, PAL_MODE_OUTPUT_PUSHPULL)

#define LED_ON palSetPad(LEDPORT, LEDPIN); palSetPad(LEDPORT_MM, LEDPIN_MM)

#define LED_OFF palClearPad(LEDPORT, LEDPIN); palClearPad(LEDPORT_MM, LEDPIN_MM)

/*
 * Port A setup.
 * Everything input with pull-up except:
 * PA0  - Normal input      (BUTTON).
 * PA2  - Alternate output  (CAN TX).
 * PA3  - Normal input      (CAN RX).
 * PA11 - Normal input      (USB DM).
 * PA12 - Normal input      (USB DP).
 */
#define VAL_GPIOACRL            0x88884B84      /*  PA7...PA0 */
#define VAL_GPIOACRH            0x888B44B8      /* PA15...PA8 */
#define VAL_GPIOAODR            0xFFFFFFFF

/*
 * Port B setup.
 * Everything input with pull-up except:
 * PB13 - Alternate output  (MMC SPI2 SCK).
 * PB14 - Normal input      (MMC SPI2 MISO).
 * PB15 - Alternate output  (MMC SPI2 MOSI).
 */
#define VAL_GPIOBCRL            0x88888888      /*  PB7...PB0 */
#define VAL_GPIOBCRH            0xB4B88888      /* PB15...PB8 */
#define VAL_GPIOBODR            0xFFFFFFFF

/*
 * Port C setup.
 * Everything input with pull-up except:
 * PC4  - Normal input because there is an external resistor.
 * PC6  - Normal input because there is an external resistor.
 * PC7  - Normal input because there is an external resistor.
 * PC10 - Push Pull output (CAN CNTRL).
 * PC11 - Push Pull output (USB DISC).
 * PC12 - Push Pull output (LED).
 */
#define VAL_GPIOCCRL            0x44848888      /*  PC7...PC0 */
#define VAL_GPIOCCRH            0x88833388      /* PC15...PC8 */
#define VAL_GPIOCODR            0xFFFFFFFF

/*
 * Port D setup.
 * Everything input with pull-up except:
 * PD0  - Normal input (XTAL).
 * PD1  - Normal input (XTAL).
 */
#define VAL_GPIODCRL            0x88888844      /*  PD7...PD0 */
#define VAL_GPIODCRH            0x88888888      /* PD15...PD8 */
#define VAL_GPIODODR            0xFFFFFFFF

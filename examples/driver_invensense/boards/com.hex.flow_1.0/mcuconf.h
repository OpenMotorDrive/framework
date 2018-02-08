#pragma once

#define STM32F3xx_MCUCONF
#define STM32_HSECLK 8000000

#define STM32_NO_INIT                       FALSE
#define STM32_PVD_ENABLE                    FALSE
#define STM32_PLS                           STM32_PLS_LEV0
#define STM32_HSI_ENABLED                   TRUE
#define STM32_LSI_ENABLED                   TRUE
#define STM32_HSE_ENABLED                   TRUE
#define STM32_LSE_ENABLED                   FALSE
#define STM32_SW                            STM32_SW_PLL
#define STM32_PLLSRC                        STM32_PLLSRC_HSE
#define STM32_PREDIV_VALUE                  1
#define STM32_PLLMUL_VALUE                  9
#define STM32_HPRE                          STM32_HPRE_DIV1
#define STM32_PPRE1                         STM32_PPRE1_DIV2
#define STM32_PPRE2                         STM32_PPRE2_DIV2
#define STM32_MCOSEL                        STM32_MCOSEL_NOCLOCK
#define STM32_ADC12PRES                     STM32_ADC12PRES_DIV1
#define STM32_ADC34PRES                     STM32_ADC34PRES_DIV1
#define STM32_USART1SW                      STM32_USART1SW_PCLK
#define STM32_USART2SW                      STM32_USART2SW_PCLK
#define STM32_USART3SW                      STM32_USART3SW_PCLK
#define STM32_UART4SW                       STM32_UART4SW_PCLK
#define STM32_UART5SW                       STM32_UART5SW_PCLK
#define STM32_I2C1SW                        STM32_I2C1SW_SYSCLK
#define STM32_I2C2SW                        STM32_I2C2SW_SYSCLK
#define STM32_TIM1SW                        STM32_TIM1SW_PCLK2
#define STM32_TIM8SW                        STM32_TIM8SW_PCLK2
#define STM32_RTCSEL                        STM32_RTCSEL_LSI
#define STM32_USB_CLOCK_REQUIRED            FALSE
#define STM32_USBPRE                        STM32_USBPRE_DIV1P5

#define STM32_SPI_USE_SPI1                  FALSE
#define STM32_SPI_USE_SPI2                  FALSE
#define STM32_SPI_USE_SPI3                  TRUE
#define STM32_SPI_SPI1_DMA_PRIORITY         1
#define STM32_SPI_SPI2_DMA_PRIORITY         1
#define STM32_SPI_SPI3_DMA_PRIORITY         1
#define STM32_SPI_SPI1_IRQ_PRIORITY         10
#define STM32_SPI_SPI2_IRQ_PRIORITY         10
#define STM32_SPI_SPI3_IRQ_PRIORITY         10
#define STM32_SPI_DMA_ERROR_HOOK(spip)      osalSysHalt("DMA failure")

#define STM32_CAN_USE_CAN1                  TRUE
#define STM32_CAN_CAN1_IRQ_PRIORITY         11

#define STM32_ST_IRQ_PRIORITY               8
#define STM32_ST_USE_TIMER                  2

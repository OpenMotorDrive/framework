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

#include <common/init.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>

static void init_clock_stm32f3_8mhz_hse(void);
static void init_clock_stm32f3_24mhz_hse(void);

void init_clock(void) {
#if defined(BOARD_CONFIG_MCU_STM32F3) && defined(BOARD_CONFIG_OSC_HSE_8MHZ)
    init_clock_stm32f3_8mhz_hse();
#elif defined(BOARD_CONFIG_MCU_STM32F3) && defined(BOARD_CONFIG_OSC_HSE_24MHZ)
#else
    #error "Could not find valid clock config"
#endif
}

static void init_clock_stm32f3_8mhz_hse(void)
{
    rcc_osc_on(RCC_HSE);
    rcc_wait_for_osc_ready(RCC_HSE);
    rcc_set_sysclk_source(RCC_CFGR_SW_HSE);
    rcc_wait_for_sysclk_status(RCC_HSE);

    rcc_osc_off(RCC_PLL);
    rcc_wait_for_osc_not_ready(RCC_PLL);
    rcc_set_prediv(RCC_CFGR2_PREDIV_NODIV); // 8 Mhz
    rcc_set_pll_source(RCC_CFGR_PLLSRC_HSE_PREDIV);
    rcc_set_pll_multiplier(RCC_CFGR_PLLMUL_PLL_IN_CLK_X9); // 72 MHz

    rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE); // 72 MHz
    rcc_ahb_frequency = 72000000;

    rcc_set_ppre1(RCC_CFGR_PPRE1_DIV_2); // 36 MHz
    rcc_apb1_frequency = 36000000;

    rcc_set_ppre2(RCC_CFGR_PPRE2_DIV_NONE); // 72 MHz
    rcc_apb2_frequency = 72000000;

    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    flash_set_ws(FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2WS);
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(RCC_PLL);
}

static void init_clock_stm32f3_24mhz_hse(void)
{
    rcc_osc_on(RCC_HSE);
    rcc_wait_for_osc_ready(RCC_HSE);
    rcc_set_sysclk_source(RCC_CFGR_SW_HSE);
    rcc_wait_for_sysclk_status(RCC_HSE);

    rcc_osc_off(RCC_PLL);
    rcc_wait_for_osc_not_ready(RCC_PLL);
    rcc_set_prediv(RCC_CFGR2_PREDIV_DIV3); // 24 -> 8 Mhz
    rcc_set_pll_source(RCC_CFGR_PLLSRC_HSE_PREDIV);
    rcc_set_pll_multiplier(RCC_CFGR_PLLMUL_PLL_IN_CLK_X9); // 72 MHz

    rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE); // 72 MHz
    rcc_ahb_frequency = 72000000;

    rcc_set_ppre1(RCC_CFGR_PPRE1_DIV_2); // 36 MHz
    rcc_apb1_frequency = 36000000;

    rcc_set_ppre2(RCC_CFGR_PPRE2_DIV_NONE); // 72 MHz
    rcc_apb2_frequency = 72000000;

    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    flash_set_ws(FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2WS);
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(RCC_PLL);
}

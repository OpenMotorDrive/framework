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

#include <common/timing.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

static uint32_t counts_per_us;
static uint32_t counts_per_ms;
static volatile uint32_t system_millis;

void sys_tick_handler(void);

void timing_init(void)
{
    counts_per_ms = rcc_ahb_frequency/1000UL;
    counts_per_us = rcc_ahb_frequency/1000000UL;
    systick_set_reload(counts_per_ms-1); // 1 ms
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

uint32_t millis(void) {
    return system_millis;
}

uint32_t micros(void) {
    uint32_t ms;
    uint32_t counter;

    do {
        ms = system_millis;
        counter = systick_get_value();
        if (systick_get_countflag()) {
            system_millis++;
        }
    } while(system_millis != ms);

    return ms*1000UL + (counts_per_ms-counter)/counts_per_us;
}

void usleep(uint32_t delay) {
    uint32_t tbegin = micros();
    while (micros()-tbegin < delay);
}

void sys_tick_handler(void)
{
    if (systick_get_countflag()) {
        system_millis++;
    }
}

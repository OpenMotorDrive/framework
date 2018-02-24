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

#include <common/ctor.h>
#include "timing.h"
#include <ch.h>
#include <hal.h>
#include <modules/worker_thread/worker_thread.h>
#include <modules/uavcan_debug/uavcan_debug.h>

#ifndef TIMING_WORKER_THREAD
#error Please define TIMING_WORKER_THREAD in framework_conf.h.
#endif

#define WT TIMING_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct timing_state {
    uint64_t update_seconds;
    systime_t update_systime;
} timing_state[2];

static volatile uint8_t timing_state_idx;

static struct worker_thread_timer_task_s timing_state_update_task;

static void timing_state_update_task_func(struct worker_thread_timer_task_s* task);

// This task must run more frequently than the system timer wraps
// For a 16 bit timer running at 10KHz, the wraparound interval is 6.5536 seconds
RUN_AFTER(WORKER_THREADS_INIT) {
    worker_thread_add_timer_task(&WT, &timing_state_update_task, timing_state_update_task_func, NULL, S2US(5), true);
}

/*
uint32_t millis(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    systime_t delta_ticks = systime_now - timing_state[idx].update_systime;
    // assume CH_CFG_ST_FREQUENCY > 1000
    micros_time_t delta_ms = delta_ticks / (CH_CFG_ST_FREQUENCY / 1000UL);
    return ((micros_time_t)timing_state[idx].update_seconds * 1000UL) + delta_ms;
}
*/

micros_time_t micros(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    systime_t delta_ticks = systime_now - timing_state[idx].update_systime;
    // assume CH_CFG_ST_FREQUENCY) <= 1e6
    micros_time_t delta_us = delta_ticks * (1000000UL / CH_CFG_ST_FREQUENCY);
    return ((micros_time_t)timing_state[idx].update_seconds * 1000000UL) + delta_us;
}

// provide a 64bit version of micros independent of MICROS_TIME_RESOLUTION
uint64_t micros64(void) {
#if (MICROS_TIME_RESOLUTION == 32)
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    systime_t delta_ticks = systime_now - timing_state[idx].update_systime;
    // assume CH_CFG_ST_FREQUENCY) <= 1e6
    uint32_t delta_us = delta_ticks * (1000000UL / CH_CFG_ST_FREQUENCY);
    return (timing_state[idx].update_seconds * 1000000UL) + delta_us;
#elif (MICROS_TIME_RESOLUTION == 64)
    return micros();
#endif
}

void usleep(micros_time_t delay) {
    micros_time_t tbegin = micros();
    while (micros() - tbegin < delay);
}

static void timing_state_update_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    CONFIG_LED;
    LED_TOGGLE;

    uint8_t next_timing_state_idx = (timing_state_idx+1) % 2;

    systime_t systime_now = chVTGetSystemTimeX();
    systime_t delta_ticks = systime_now - timing_state[timing_state_idx].update_systime;

    timing_state[next_timing_state_idx].update_seconds = timing_state[timing_state_idx].update_seconds + delta_ticks / CH_CFG_ST_FREQUENCY;
    timing_state[next_timing_state_idx].update_systime = systime_now - (delta_ticks % CH_CFG_ST_FREQUENCY);

    uint32_t update_seconds = timing_state[next_timing_state_idx].update_seconds;
    systime_t update_systime = timing_state[next_timing_state_idx].update_systime;

    uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_INFO, "",
                          "micros: %u, /1000: %u, update_seconds: %u, update_systime: %u",
                          micros(), (uint32_t)(micros64()/1000), update_seconds, update_systime);

    timing_state_idx = next_timing_state_idx;
}

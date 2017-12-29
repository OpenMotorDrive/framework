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
#include <modules/worker_thread/worker_thread.h>

#ifndef TIMING_WORKER_THREAD
#error Please define TIMING_WORKER_THREAD in worker_threads_conf.h.
#endif

#define WT TIMING_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct {
    uint64_t update_seconds;
    systime_t update_systime;
} timing_state[2];

static volatile uint8_t timing_state_idx;

static struct worker_thread_timer_task_s timing_state_update_task;

static void timing_state_update_task_func(struct worker_thread_timer_task_s* task);

RUN_AFTER(WORKER_THREADS_INIT) {
    worker_thread_add_timer_task(&WT, &timing_state_update_task, timing_state_update_task_func, NULL, S2ST(10), true);
}

uint32_t millis(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    uint32_t delta_ticks = systime_now-timing_state[idx].update_systime;
    uint32_t delta_ms = delta_ticks / (CH_CFG_ST_FREQUENCY/1000);
    return ((uint32_t)timing_state[idx].update_seconds*1000) + delta_ms;
}

uint32_t micros(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    uint32_t delta_ticks = systime_now-timing_state[idx].update_systime;
    uint32_t delta_us = delta_ticks / (CH_CFG_ST_FREQUENCY/1000000);
    return ((uint32_t)timing_state[idx].update_seconds*1000000) + delta_us;
}

uint64_t micros64(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    uint32_t delta_ticks = systime_now-timing_state[idx].update_systime;
    uint32_t delta_us = delta_ticks / (CH_CFG_ST_FREQUENCY/1000000);
    return (timing_state[idx].update_seconds*1000000) + delta_us;
}

void usleep(uint32_t delay) {
    uint32_t tbegin = micros();
    while (micros()-tbegin < delay);
}

static void timing_state_update_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    uint8_t next_timing_state_idx = (timing_state_idx+1) % 2;

    systime_t systime_now = chVTGetSystemTimeX();
    uint32_t delta_ticks = systime_now-timing_state[timing_state_idx].update_systime;

    timing_state[next_timing_state_idx].update_seconds = timing_state[timing_state_idx].update_seconds + delta_ticks / CH_CFG_ST_FREQUENCY;
    timing_state[next_timing_state_idx].update_systime = systime_now - (delta_ticks % CH_CFG_ST_FREQUENCY);

    timing_state_idx = next_timing_state_idx;
}

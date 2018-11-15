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
#include <ch.h>
#include <modules/worker_thread/worker_thread.h>
#include <modules/uavcan_debug/uavcan_debug.h>

#ifndef LOAD_MEASUREMENT_WORKER_THREAD
#error Please define LOAD_MEASUREMENT_WORKER_THREAD in framework_conf.h.
#endif

#define WT LOAD_MEASUREMENT_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct worker_thread_timer_task_s load_print_task;
static void load_print_task_func(struct worker_thread_timer_task_s* task);

static time_measurement_t cumtime;

RUN_AFTER(WORKER_THREADS_INIT) {
    chTMStartMeasurementX(&cumtime);
    worker_thread_add_timer_task(&WT, &load_print_task, load_print_task_func, NULL, S2ST(5), true);
}

static void load_print_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;

    chTMStopMeasurementX(&cumtime);
    chTMStartMeasurementX(&cumtime);

    thread_t* thread = chRegFirstThread();
    while(thread) {
        uint32_t load = 10000*thread->stats.cumulative/cumtime.cumulative;
        uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_INFO, "load", "%s: %u.%02u%% %u", thread->name, load/100, load%100, (unsigned)thread->stats.cumulative);
        thread->stats.cumulative = 0;
        thread = chRegNextThread(thread);
    }
    cumtime.cumulative = 0;
}

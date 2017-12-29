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

#ifndef STACK_MEASUREMENT_WORKER_THREAD
#error Please define STACK_MEASUREMENT_WORKER_THREAD in framework_conf.h.
#endif

#define WT STACK_MEASUREMENT_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct worker_thread_timer_task_s stack_print_task;
static void stack_print_task_func(struct worker_thread_timer_task_s* task);

RUN_AFTER(WORKER_THREADS_INIT) {
    worker_thread_add_timer_task(&WT, &stack_print_task, stack_print_task_func, NULL, S2ST(5), true);
}

extern uint8_t __process_stack_base__;
extern uint8_t __main_stack_base__;

static void _print_thread_free_stack(const char* name, void* stack_base) {

    uint8_t* startp = (uint8_t*)stack_base;

    size_t n = 0;
    while (true) {
        if (*(startp+n) != CH_DBG_STACK_FILL_VALUE) {
            break;
        }
        n++;
    }
    uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_INFO, "", "%s : %u free", name, n);
}

static void print_thread_free_stack(thread_t* thread) {
    if (!thread->wabase) {
        _print_thread_free_stack(thread->name, &__process_stack_base__);
    } else if(thread->wabase == ch_idle_thread_wa) {
        _print_thread_free_stack(thread->name, thread->wabase);
    } else {
        _print_thread_free_stack(thread->name, (uint8_t*)thread->wabase + sizeof(thread_t));
    }
}

static void print_exception_free_stack(void) {
    _print_thread_free_stack("exceptions", &__main_stack_base__);
}

static void stack_print_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;

    thread_t* thread = chRegFirstThread();
    while(thread) {
        print_thread_free_stack(thread);
        thread = chRegNextThread(thread);
    }
    print_exception_free_stack();
}

/*
 *    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <ch.h>
#include <modules/uavcan_debug/uavcan_debug.h>
#include <modules/param/param.h>
#include <common/helpers.h>
#include <string.h>

int main(void) {
    for (volatile uint16_t* addr = (volatile uint16_t*)0x40006400; (size_t)addr <= 0x400067FF; addr++) {
        int16_t diffs[10];
        for (uint8_t i=0; i<10; i++) {
            uint16_t reg_before_delay = *addr;
            chThdSleep(MS2ST(10));
            uint16_t reg_after_delay = *addr;
            diffs[i] = reg_after_delay-reg_before_delay;
        }

        float diff_mean = 0;
        for (uint8_t i=0; i<10; i++) {
            diff_mean += diffs[i];
        }
        diff_mean /= 10;

        float diff_sigma = 0;
        for (uint8_t i=0; i<10; i++) {
            diff_sigma += SQ(diffs[i]-diff_mean);
        }
        diff_sigma = sqrtf(diff_sigma/9);

        if (fabsf(diff_mean) > 1000 && diff_sigma < 1000) {
            uavcan_send_debug_msg(LOG_LEVEL_WARNING, "", "candidate 0x%08X, diff=%f, diff_sigma=%f", addr, diff_mean, diff_sigma);
        } else {
            uavcan_send_debug_msg(LOG_LEVEL_DEBUG, "", "candidate 0x%08X, diff=%f, diff_sigma=%f", addr, diff_mean, diff_sigma);
        }
    }
    uavcan_send_debug_msg(LOG_LEVEL_DEBUG, "", "fin.");
    chThdSleep(TIME_INFINITE);

    return 0;
}

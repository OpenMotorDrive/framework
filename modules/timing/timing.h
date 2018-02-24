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

#pragma once

#include <stdint.h>
#include <framework_conf.h>

#ifndef MICROS_TIME_RESOLUTION
#define MICROS_TIME_RESOLUTION 64
#endif

#if (MICROS_TIME_RESOLUTION == 32)
typedef uint32_t micros_time_t;
#elif MICROS_TIME_RESOLUTION == 64
typedef uint64_t micros_time_t;
#else
#error "invalid MICROS_TIME_RESOLUTION setting"
#endif

#define MICROS_INFINITE ((micros_time_t)-1)

#define MS2US(msec) (msec * 1000UL)
#define S2US(sec) (sec * 1000000UL)

//uint32_t millis(void);
micros_time_t micros(void);
uint64_t micros64(void);
void usleep(micros_time_t delay);

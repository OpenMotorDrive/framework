#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ch.h"
#include "hal.h"
#include <modules/pubsub/pubsub.h>
/* Timer TIM1 is the only one speifically designed for monitoring a PWM input.
 * The STM32F302R8 and STM32F302K8 do not have TM2 but not TIM3 or TIM4
 * Timers 15, 16, & 17 are available for interval measurements.
 *
 * PA8 (5Volt tolerant) = TIM1_CH1 / AF6: for UQFN32 it is pin 18. For the LQFP64 it is pin 41.
 * On the charger board this is CN9 pin 8 and labeled D7.
 * On the charger board the nearest GND is CN7 pin 20
 * On the charger board the nearest +5V is CN7 pin 18
 *
 */

struct timer_input_capture_msg_s {
    systime_t timestamp;
    icucnt_t width;
    icucnt_t period;
};

bool timer_input_capture_publisher_enable_T1(struct pubsub_topic_s* topic);
bool timer_input_capture_publisher_enable_T1_oneshot(struct pubsub_topic_s* topic); // Masks the interrupt until a subscriber re-enables it.
void timer_input_capture_publisher_disable_T1(void);
void timer_input_capture_publisher_unmask_T1(void);

#pragma once

#include <stdint.h>

#define FW_EXT_IRQ_PORT(x)     EXT_MODE_ ## x

typedef void (*ext_irq_cb)(void);

enum {
    FW_EXT_IRQ_LOW_LEVEL = 0,
    FW_EXT_IRQ_FALLING,
    FW_EXT_IRQ_RISING,
    FW_EXT_IRQ_BOTH
};


struct pubsub_topic_s* enable_ext_irq(uint32_t gpio_port, uint8_t pin_int_num, uint8_t mode);
void disable_ext_irq(uint8_t pin_int_num);
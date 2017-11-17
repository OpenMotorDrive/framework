

#include <hal.h>
#include "ext_irq.h"

static ext_irq_cb ext_irq_cb_list[22];
static EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};

static void ext_irq_common_handler(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    if (ext_irq_cb_list[channel] != NULL) {
        ext_irq_cb_list[channel]();
    }
}

void enable_ext_irq(uint32_t gpio_port, uint8_t pin_int_num, uint8_t mode, ext_irq_cb cb)
{
    extStop(&EXTD1);
    switch(mode) {
        case FW_EXT_IRQ_LOW_LEVEL:
            extcfg.channels[pin_int_num].mode = EXT_CH_MODE_LOW_LEVEL;
            break;
        case FW_EXT_IRQ_FALLING:
            extcfg.channels[pin_int_num].mode = EXT_CH_MODE_FALLING_EDGE;
            break;
        case FW_EXT_IRQ_RISING:
            extcfg.channels[pin_int_num].mode = EXT_CH_MODE_RISING_EDGE;
            break;
        case FW_EXT_IRQ_BOTH:
            extcfg.channels[pin_int_num].mode = EXT_CH_MODE_BOTH_EDGES;
            break;
    }
    extcfg.channels[pin_int_num].mode |= EXT_CH_MODE_AUTOSTART | gpio_port;
    ext_irq_cb_list[pin_int_num] = cb;
    extcfg.channels[pin_int_num].cb = ext_irq_common_handler;
    extStart(&EXTD1, &extcfg);
}

void disable_ext_irq(uint8_t pin_int_num)
{
    extStop(&EXTD1);
    extcfg.channels[pin_int_num].mode = EXT_CH_MODE_DISABLED;
    extcfg.channels[pin_int_num].cb = NULL;
    extStart(&EXTD1, &extcfg);
}

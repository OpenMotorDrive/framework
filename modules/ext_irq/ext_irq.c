

#include <hal.h>
#include "ext_irq.h"
#include <modules/pubsub/pubsub.h>
#include <common/ctor.h>
#include <common/helpers.h>
#include <string.h>

struct ext_irq_topic_list_s {
    uint32_t channel;
    struct pubsub_topic_s topic;
    struct ext_irq_topic_list_s* next;
} ext_irq_topic_list; //topic for each channel

struct ext_irq_instance_s {
    EXTDriver *drv;
    struct ext_irq_topic_list_s *ext_irq_list_head;
    mutex_t lock;
    thread_t *ext_irq_thd;
} *exti_instance = NULL;

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
MEMORYPOOL_DECL(ext_irq_topic_list_pool, sizeof(struct ext_irq_topic_list_s), chCoreAllocAlignedI);

static THD_WORKING_AREA(ext_irq_thd_wa, 128);
static THD_FUNCTION(ext_irq_thd_func, arg) {
    (void)arg;
    struct ext_irq_topic_list_s* ext_irq_list_item;
    uint32_t evt_mask;   
    static volatile uint16_t irq_cnt;
    while(true) {
        evt_mask = chEvtWaitAny((eventmask_t)0xFFFF);
        irq_cnt++;
        chMtxLock(&exti_instance->lock);
        ext_irq_list_item = exti_instance->ext_irq_list_head;
        while (ext_irq_list_item) {
            if ((1<<ext_irq_list_item->channel) & evt_mask) {
                pubsub_publish_message(&ext_irq_list_item->topic, 0, NULL, NULL);
            }
            ext_irq_list_item = ext_irq_list_item->next;
        }
        chMtxUnlock(&exti_instance->lock);

    }
}

static void ext_irq_common_handler(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    static volatile uint16_t irq_cnt;
    if (exti_instance == NULL) {
        return;
    }
    chSysLockFromISR();
    chEvtSignalI(exti_instance->ext_irq_thd, 1<<channel);
    irq_cnt++;
    chSysUnlockFromISR();
}
static void ext_irq_init(void);

RUN_AFTER(CH_SYS_INIT) {
    ext_irq_init();
}

static void ext_irq_init(void)
{
    exti_instance = chHeapAllocAligned(NULL, sizeof(struct ext_irq_instance_s), 8);
    if (!(exti_instance = chCoreAllocAligned(sizeof(struct ext_irq_instance_s), PORT_WORKING_AREA_ALIGN))) { goto fail; }
    memset(exti_instance, 0, sizeof(struct ext_irq_instance_s));
    chMtxObjectInit(&exti_instance->lock);
    if (!(exti_instance->ext_irq_thd = chThdCreateStatic(ext_irq_thd_wa, sizeof(ext_irq_thd_wa), HIGHPRIO-2, ext_irq_thd_func, exti_instance))) { goto fail; }
    exti_instance->drv = &EXTD1;
    return;
fail:
    chSysHalt("EXT IRQ Init Failed!");
}

struct pubsub_topic_s* enable_ext_irq(uint32_t gpio_port, uint8_t pin_int_num, uint8_t mode)
{
    if(!exti_instance) {
        return NULL;
    }
    chMtxLock(&exti_instance->lock);
    extStop(exti_instance->drv);
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

    //Append irq topic request
    struct ext_irq_topic_list_s* ext_irq_list_item = exti_instance->ext_irq_list_head;
    while (ext_irq_list_item && ext_irq_list_item->channel != pin_int_num) {
        ext_irq_list_item = ext_irq_list_item->next;
    }
    if (ext_irq_list_item) {
        goto start_and_return;
    }

    ext_irq_list_item = chPoolAlloc(&ext_irq_topic_list_pool);
    if (!ext_irq_list_item) {
        return NULL;
    }

    ext_irq_list_item->channel = pin_int_num;
    pubsub_init_topic(&ext_irq_list_item->topic, NULL);

    LINKED_LIST_APPEND(struct ext_irq_topic_list_s, exti_instance->ext_irq_list_head, ext_irq_list_item);

    chMtxUnlock(&exti_instance->lock);

start_and_return:
    extcfg.channels[pin_int_num].cb = ext_irq_common_handler;
    extStart(exti_instance->drv, &extcfg);
    return &ext_irq_list_item->topic;
}

void disable_ext_irq(uint8_t pin_int_num)
{
    if(!exti_instance) {
        return;
    }
    extStop(exti_instance->drv);
    extcfg.channels[pin_int_num].mode = EXT_CH_MODE_DISABLED;
    extcfg.channels[pin_int_num].cb = NULL;
    //TODO: free allocated topic as well
    extStart(exti_instance->drv, &extcfg);
}

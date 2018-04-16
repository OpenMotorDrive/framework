#include "pin_change_publisher.h"
#include <common/ctor.h>
#include <common/helpers.h>
#include <modules/worker_thread/worker_thread.h>

#include <hal.h>
#include <ch.h>

#ifndef PIN_CHANGE_PUBLISHER_QUEUE_DEPTH
#define PIN_CHANGE_PUBLISHER_QUEUE_DEPTH 16
#endif

#ifndef PIN_CHANGE_PUBLISHER_WORKER_THREAD
#error Please define PIN_CHANGE_PUBLISHER_WORKER_THREAD in framework_conf.h.
#endif

#define WT PIN_CHANGE_PUBLISHER_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)


struct pin_change_publisher_topic_s {
    expchannel_t channel;
    bool mask_until_handled;
    struct pubsub_topic_s* topic;
    struct pin_change_publisher_topic_s* next;
};

struct worker_thread_publisher_task_s publisher_task;

static struct pin_change_publisher_topic_s *irq_topic_list_head;

static EXTConfig extcfg;

RUN_ON(PUBSUB_TOPIC_INIT) {
    extStart(&EXTD1, &extcfg);
    worker_thread_add_publisher_task(&WT, &publisher_task, sizeof(struct pin_change_msg_s), PIN_CHANGE_PUBLISHER_QUEUE_DEPTH);
}

MEMORYPOOL_DECL(pin_change_publisher_topic_list_pool, sizeof(struct pin_change_publisher_topic_s), chCoreAllocAlignedI);

static bool pin_change_publisher_enable_pin_with_mask_option(uint32_t line, enum pin_change_type_t mode, struct pubsub_topic_s* topic, bool mask_until_handled);
static struct pin_change_publisher_topic_s* pin_change_publisher_find_irq_topic(expchannel_t channel);
static void pin_change_publisher_common_handler(EXTDriver *extp, expchannel_t channel);
static bool pin_change_publisher_get_mode(uint32_t line, enum pin_change_type_t mode, uint32_t* ret);

bool pin_change_publisher_enable_pin(uint32_t line, enum pin_change_type_t mode, struct pubsub_topic_s* topic) {
    bool mask_until_handled = false;
    return pin_change_publisher_enable_pin_with_mask_option(line, mode, topic, mask_until_handled);
}

bool pin_change_publisher_enable_pin_oneshot(uint32_t line, enum pin_change_type_t mode, struct pubsub_topic_s* topic) {
    bool mask_until_handled = true;
    return pin_change_publisher_enable_pin_with_mask_option(line, mode, topic, mask_until_handled);
}

static bool pin_change_publisher_enable_pin_with_mask_option(uint32_t line, enum pin_change_type_t mode, struct pubsub_topic_s* topic, bool mask_until_handled) {
    chSysLock();

    expchannel_t channel = PAL_PAD(line);

    if (!topic || pin_change_publisher_find_irq_topic(channel) || extcfg.channels[channel].mode != EXT_CH_MODE_DISABLED) {
        chSysUnlock();
        return false;
    }

    uint32_t pin_change_publisher_mode;
    if (!pin_change_publisher_get_mode(line, mode, &pin_change_publisher_mode)) {
        chSysUnlock();
        return false;
    }

    struct pin_change_publisher_topic_s* irq_topic = chPoolAllocI(&pin_change_publisher_topic_list_pool);
    if (!irq_topic) {
        chSysUnlock();
        return false;
    }

    irq_topic->channel = channel;
    irq_topic->topic = topic;
    irq_topic->mask_until_handled = mask_until_handled;

    LINKED_LIST_APPEND(struct pin_change_publisher_topic_s, irq_topic_list_head, irq_topic);

    const EXTChannelConfig channel_conf = {pin_change_publisher_mode, pin_change_publisher_common_handler};
    extSetChannelModeI(&EXTD1, channel, &channel_conf);

    chSysUnlock();

    return true;
}

void pin_change_publisher_disable_pin(uint32_t line) {
    chSysLock();

    expchannel_t channel = PAL_PAD(line);

    struct pin_change_publisher_topic_s* irq_topic = pin_change_publisher_find_irq_topic(channel);

    if (irq_topic) {
        LINKED_LIST_REMOVE(struct pin_change_publisher_topic_s, irq_topic_list_head, irq_topic);
    }

    if (extcfg.channels[channel].mode != EXT_CH_MODE_DISABLED) {
        extChannelDisable(&EXTD1, channel);
    }

    chSysUnlock();
}

static struct pin_change_publisher_topic_s* pin_change_publisher_find_irq_topic(expchannel_t channel) {
    struct pin_change_publisher_topic_s* irq_topic = irq_topic_list_head;
    while(irq_topic) {
        if (irq_topic->channel == channel) {
            return irq_topic;
        }
        irq_topic = irq_topic->next;
    }
    return NULL;
}

static void pin_change_publisher_common_handler(EXTDriver *extp, expchannel_t channel) {
    (void)extp;
    struct pin_change_publisher_topic_s* irq_topic = pin_change_publisher_find_irq_topic(channel);

    if (irq_topic) {
        chSysLockFromISR();
        if (irq_topic->mask_until_handled) {
            extChannelDisableI(extp, channel);
        }
        struct pin_change_msg_s msg = {chVTGetSystemTimeX()};
        worker_thread_publisher_task_publish_I(&publisher_task, irq_topic->topic, sizeof(struct pin_change_msg_s), pubsub_copy_writer_func, &msg);
        chSysUnlockFromISR();
    }
}

static bool pin_change_publisher_get_mode(uint32_t line, enum pin_change_type_t mode, uint32_t* ret) {
    uint32_t ext_channel_mode;

    switch((uint32_t)PAL_PORT(line)) {
#if STM32_HAS_GPIOA
        case (uint32_t)GPIOA:
            ext_channel_mode = EXT_MODE_GPIOA;
            break;
#endif
#if STM32_HAS_GPIOB
        case (uint32_t)GPIOB:
            ext_channel_mode = EXT_MODE_GPIOB;
            break;
#endif
#if STM32_HAS_GPIOC
        case (uint32_t)GPIOC:
            ext_channel_mode = EXT_MODE_GPIOC;
            break;
#endif
#if STM32_HAS_GPIOD
        case (uint32_t)GPIOD:
            ext_channel_mode = EXT_MODE_GPIOD;
            break;
#endif
#if STM32_HAS_GPIOE
        case (uint32_t)GPIOE:
            ext_channel_mode = EXT_MODE_GPIOE;
            break;
#endif
#if STM32_HAS_GPIOF
        case (uint32_t)GPIOF:
            ext_channel_mode = EXT_MODE_GPIOF;
            break;
#endif
#if STM32_HAS_GPIOG
        case (uint32_t)GPIOG:
            ext_channel_mode = EXT_MODE_GPIOG;
            break;
#endif
#if STM32_HAS_GPIOH
        case (uint32_t)GPIOH:
            ext_channel_mode = EXT_MODE_GPIOH;
            break;
#endif
#if STM32_HAS_GPIOI
        case (uint32_t)GPIOI:
            ext_channel_mode = EXT_MODE_GPIOI;
            break;
#endif
        default:
            return false;
    }

    switch(mode) {
        case PIN_CHANGE_TYPE_LOW_LEVEL:
            ext_channel_mode |= EXT_CH_MODE_LOW_LEVEL;
            break;
        case PIN_CHANGE_TYPE_FALLING:
            ext_channel_mode |= EXT_CH_MODE_FALLING_EDGE;
            break;
        case PIN_CHANGE_TYPE_RISING:
            ext_channel_mode |= EXT_CH_MODE_RISING_EDGE;
            break;
        case PIN_CHANGE_TYPE_BOTH:
            ext_channel_mode |= EXT_CH_MODE_BOTH_EDGES;
            break;
        default:
            return false;
    }

    *ret = ext_channel_mode;

    return true;
}

void pin_change_publisher_unmask_pin(uint32_t line) {
    expchannel_t channel = PAL_PAD(line);

    extChannelEnable(&EXTD1, channel);
}

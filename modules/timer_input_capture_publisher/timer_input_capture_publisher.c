#include <common/ctor.h>
#include <common/helpers.h>
#include <platform.h>
#include <modules/worker_thread/worker_thread.h>
#include "timer_input_capture_publisher.h"

#ifndef TIMER_INPUT_CAPTURE_PUBLISHER_QUEUE_DEPTH
#define TIMER_INPUT_CAPTURE_PUBLISHER_QUEUE_DEPTH 2
#endif

#ifndef TIMER_INPUT_CAPTURE_PUBLISHER_WORKER_THREAD
#error Please define TIMER_INPUT_CAPTURE_PUBLISHER_WORKER_THREAD in framework_conf.h.
#endif

#define WT TIMER_INPUT_CAPTURE_PUBLISHER_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)


struct timer_input_capture_publisher_topic_s {
    expchannel_t channel;
    bool mask_until_handled;
    struct pubsub_topic_s* topic;
    struct timer_input_capture_publisher_topic_s* next;
};

static struct worker_thread_publisher_task_s publisher_task;

struct timer_input_capture_msg_s timer_input_capture_msg;
static ICUDriver icu_T1;
struct timer_input_capture_publisher_topic_s* irq_topic_T1 = NULL;

RUN_ON(PUBSUB_TOPIC_INIT) {
    worker_thread_add_publisher_task(&WT, &publisher_task, sizeof(struct timer_input_capture_msg_s), TIMER_INPUT_CAPTURE_PUBLISHER_QUEUE_DEPTH);
}

MEMORYPOOL_DECL(timer_input_capture_publisher_topic_list_pool, sizeof(struct timer_input_capture_publisher_topic_s), chCoreAllocAlignedI);

static bool timer_input_capture_publisher_enable_T1_with_mask_option(struct pubsub_topic_s* topic, bool mask_until_handled);

void icuperiodcb(ICUDriver *icup) {
//    __asm__("bkpt");
    if (irq_topic_T1 != NULL) {
        chSysLockFromISR();
        if (irq_topic_T1->mask_until_handled) {
            icu_lld_disable_notifications(&icu_T1);
        }
        timer_input_capture_msg.period = icuGetPeriodX(icup);
        timer_input_capture_msg.width  = icuGetWidthX(icup);
        timer_input_capture_msg.timestamp = chVTGetSystemTimeX();
        worker_thread_publisher_task_publish_I(&publisher_task, irq_topic_T1->topic, sizeof(struct timer_input_capture_msg_s), pubsub_copy_writer_func, &timer_input_capture_msg);
        chSysUnlockFromISR();
    }
}

bool timer_input_capture_publisher_enable_T1(struct pubsub_topic_s* topic) {
    bool mask_until_handled = false;
    return timer_input_capture_publisher_enable_T1_with_mask_option(topic, mask_until_handled);
}

bool timer_input_capture_publisher_enable_T1_oneshot(struct pubsub_topic_s* topic) {
    bool mask_until_handled = true;
    return timer_input_capture_publisher_enable_T1_with_mask_option(topic, mask_until_handled);
}

static bool timer_input_capture_publisher_enable_T1_with_mask_option(struct pubsub_topic_s* topic, bool mask_until_handled) {
    eventLog_debugEvent("ic_en");
    chSysLock();
//    __asm__("bkpt");
    if (!topic) {
        chSysUnlock();
        return false;
    }

    irq_topic_T1 = chPoolAllocI(&timer_input_capture_publisher_topic_list_pool);
    if (!irq_topic_T1) {
        chSysUnlock();
        return false;
    }

    irq_topic_T1->channel = 1;
    irq_topic_T1->topic = topic;
    irq_topic_T1->mask_until_handled = mask_until_handled;

    const ICUConfig icucfg = {
      ICU_INPUT_ACTIVE_HIGH,
      100000,                                    /* 1MHz ICU clock frequency.   */
      NULL,                                       /* icuwidthcb */
      icuperiodcb,
      NULL,
      ICU_CHANNEL_1,
      0
    };

    palSetPadMode(GPIOA, 8, PAL_MODE_ALTERNATE(6));
    __asm__("bkpt");
    icuStart(&ICUD1, &icucfg);
//    icuStartCapture(&ICUD1);
//    icuEnableNotifications(&ICUD1);

    chSysUnlock();
    eventLog_debugEvent("-ic_en");

    return true;
}

void timer_input_capture_publisher_disable_T1(void) {
    icuDisableNotifications(&icu_T1);
}

void timer_input_capture_publisher_unmask_T1(void) {
    icuEnableNotifications(&icu_T1);
}

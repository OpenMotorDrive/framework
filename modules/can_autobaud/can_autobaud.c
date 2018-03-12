#include <common/helpers.h>
#include <modules/can/can.h>
#include <modules/worker_thread/worker_thread.h>

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
#include <modules/app_descriptor/app_descriptor.h>
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
#include <modules/boot_msg/boot_msg.h>
#endif

#ifndef CAN_AUTOBAUD_WORKER_THREAD
#error Please define CAN_AUTOBAUD_WORKER_THREAD in framework_conf.h.
#endif

#define WT CAN_AUTOBAUD_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

#define CAN_AUTOBAUD_SWITCH_INTERVAL_MS 1000

static const uint32_t valid_baudrates[] = {1000000, 500000, 250000, 125000};

static uint8_t baudrate_idx = 0;
static void autobaud_timer_task_func(struct worker_thread_timer_task_s* task);
static struct worker_thread_timer_task_s autobaud_timer_task;

static bool is_baudrate_valid(uint32_t baudrate) {
    for (uint8_t i=0; i<LEN(valid_baudrates); i++) {
        if (baudrate == valid_baudrates[i]) {
            return true;
        }
    }
    return false;
}

RUN_AFTER(CAN_INIT) {
    uint32_t canbus_baud = 1000000;
    bool canbus_autobaud_enable = true;

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
    if (is_baudrate_valid(shared_get_parameters(&shared_app_descriptor)->canbus_baudrate)) {
        canbus_baud = shared_get_parameters(&shared_app_descriptor)->canbus_baudrate;
    }

    if (shared_get_parameters(&shared_app_descriptor)->canbus_disable_auto_baud) {
        canbus_autobaud_enable = false;
    }
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
    bool boot_msg_valid = get_boot_msg_valid();

    if (boot_msg_valid && is_baudrate_valid(boot_msg.canbus_info.baudrate)) {
        canbus_baud = boot_msg.canbus_info.baudrate;
        canbus_autobaud_enable = false;
    }
#endif

    for (uint8_t i=0; i<LEN(valid_baudrates); i++) {
        if (canbus_baud == valid_baudrates[i]) {
            baudrate_idx = i;
            break;
        }
    }

    struct can_instance_s* can_instance = NULL;
    while (can_iterate_instances(&can_instance)) {
        can_start(can_instance, canbus_autobaud_enable, true, canbus_baud);
    }

    if (canbus_autobaud_enable) {
        worker_thread_add_timer_task(&WT, &autobaud_timer_task, autobaud_timer_task_func, NULL, CAN_AUTOBAUD_SWITCH_INTERVAL_MS, false);
    }
}

static void autobaud_timer_task_func(struct worker_thread_timer_task_s* task) {
    baudrate_idx = (baudrate_idx + 1) % LEN(valid_baudrates);

    bool autobaud_complete = true;
    struct can_instance_s* can_instance = NULL;
    while (can_iterate_instances(&can_instance)) {
        if (can_get_baudrate_confirmed(can_instance)) {
            can_set_silent_mode(can_instance, false);
        } else {
            can_set_baudrate(can_instance, valid_baudrates[baudrate_idx]);
            autobaud_complete = false;
        }
    }

    if (!autobaud_complete) {
        worker_thread_timer_task_reschedule(&WT, task, CAN_AUTOBAUD_SWITCH_INTERVAL_MS);
    }
}

#include <modules/worker_thread/worker_thread.h>
#include <modules/uavcan_debug/uavcan_debug.h>

#include <ch.h>

#include <hal.h>


#include "usbcfg.h"
#include <shell.h>
#include <shell_cmd.h>
#include <chprintf.h>
#include <stdlib.h>     /* atoi */


#define WT hpwork_thread
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct worker_thread_timer_task_s blink_led_green_task;
static struct worker_thread_timer_task_s blink_led_red_task;

static void blink_led_green_task_func(struct worker_thread_timer_task_s* task);
static void blink_led_red_task_func(struct worker_thread_timer_task_s* task);

// handle request to change blink speed on green led
void cmd_blinkspeed(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    uint16_t millis = 500;
    if (argc != 1) {
        chprintf(chp, "Usage: blinkspeed speed [ms]\r\n");
        return;
    }
    millis = atoi(argv[0]);
    chprintf(chp, "Got speed [%d]\r\n", millis);
    if (millis > 5000) {
        millis = 5000;
    } else if(millis < 5) {
        millis = 5;
    }
    worker_thread_timer_task_reschedule(&WT, &blink_led_green_task, MS2ST(millis));
}

static const ShellCommand commands[] = {
    {"blinkspeed", cmd_blinkspeed}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream  *)&SDU1,
  commands
};

//#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
//
//static THD_WORKING_AREA(waShellThread, 2048);
//static THD_FUNCTION(ShellThread, arg) {
//    (void)arg;
//    chRegSetThreadName("shellthread");
//    while (true) {
//        if (SDU1.config->usbp->state == USB_ACTIVE) {
//            thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
//                                                    "shell", NORMALPRIO + 1,
//                                                    shellThread, (void *)&shell_cfg1);
//            chThdWait(shelltp);               /* Waiting termination.             */
//        }
//        chThdSleepMilliseconds(1000);
//    }
//}

// Init the USB
//void usb_init(void) {
//    sduObjectInit(&SDU1);
//    sduStart(&SDU1, &serusbcfg);
//    /*
//    * Activates the USB driver and then the USB bus pull-up on D+.
//    * Note, a delay is inserted in order to not have to disconnect the cable
//    * after a reset.
//    */
//    usbDisconnectBus(serusbcfg.usbp);
//    chThdSleepMilliseconds(1500);
//    usbStart(serusbcfg.usbp, &usbcfg);
//    usbConnectBus(serusbcfg.usbp);
//    /*
//     * Shell manager initialization.
//     */
//    shellInit();
//    chThdCreateStatic(waShellThread, sizeof(waShellThread), LOWPRIO,
//                      ShellThread, NULL);
//
//}

// Hold green led state
bool led_green_on;
// Hold red led state
bool led_red_on;

// Function call after the end of init
RUN_AFTER(INIT_END) {
//    usb_init();
    led_green_on = false;
    led_red_on = false;
    // Add new tasks to worker thread
    // make led blink and report every 1000ms (repeated)
    worker_thread_add_timer_task(&WT, &blink_led_green_task, blink_led_green_task_func, NULL, MS2ST(1000), true);
    worker_thread_add_timer_task(&WT, &blink_led_red_task, blink_led_red_task_func, NULL, MS2ST(2000), true);
}


static void blink_led_green_task_func(struct worker_thread_timer_task_s* task) {
    if (led_green_on) {
        palClearPad(GPIOG, GPIOG_LED3_GREEN);
        led_green_on = false;
    } else {
        palSetPad(GPIOG, GPIOG_LED3_GREEN);
        led_green_on = true;
    }
    //uavcan_send_debug_keyvalue("LG", (unsigned long)chVTGetSystemTime());
}


static void blink_led_red_task_func(struct worker_thread_timer_task_s* task) {
    if (led_red_on) {
        palClearPad(GPIOG, GPIOG_LED4_RED);
        led_red_on = false;
    } else {
        palSetPad(GPIOG, GPIOG_LED4_RED);
        led_red_on = true;
    }
    //uavcan_send_debug_keyvalue("LR", (unsigned long)chVTGetSystemTime());
}


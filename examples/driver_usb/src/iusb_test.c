#include <modules/worker_thread/worker_thread.h>
#include <ch.h>

#include <hal.h>


#include "usbcfg.h"
#include <shell.h>
#include <shell_cmd.h>
#include <chprintf.h>
#include <stdlib.h>     /* atoi */


#define WT hpwork_thread
WORKER_THREAD_DECLARE_EXTERN(WT)


int blinkspeed = 500;
void cmd_blinkspeed(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    int speed = 500;
    if (argc != 1) {
        chprintf(chp, "Usage: blinkspeed speed [ms]\r\n");
        return;
    }
    speed = atoi(argv[0]);
    blinkspeed = speed;
}

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

    (void)arg;
    chRegSetThreadName("blinker1");
    while (true) {
        palClearPad(GPIOG, GPIOG_LED4_RED);
        chThdSleepMilliseconds(blinkspeed);
        palSetPad(GPIOG, GPIOG_LED4_RED);
        chThdSleepMilliseconds(blinkspeed);
    }
}

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread2, 128);
static THD_FUNCTION(Thread2, arg) {

    (void)arg;
    chRegSetThreadName("blinker2");
    while (true) {
        palClearPad(GPIOG, GPIOG_LED3_GREEN);
        chprintf((BaseSequentialStream  *)&SDU1, "blink\r\n");

chThdSleepMilliseconds(5000);
        palSetPad(GPIOG, GPIOG_LED3_GREEN);
        chThdSleepMilliseconds(5000);
    }
}

static THD_WORKING_AREA(waShell, 2048);

static const ShellCommand commands[] = {
    {"blinkspeed", cmd_blinkspeed}
};

/*
 *
 */
static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream  *)&SDU1,
  commands
};

void usb_init(void) {
    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);
    /*
    * Activates the USB driver and then the USB bus pull-up on D+.
    * Note, a delay is inserted in order to not have to disconnect the cable
    * after a reset.
    */
    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1500);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);
    
    /*
     * Shell manager initialization.
     */
    shellInit();
    chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO,
                    shellThread, (void *)&shell_cfg1);

    chThdCreateStatic(waThread1, sizeof(waThread1),
                      NORMALPRIO + 10, Thread1, NULL);
    chThdCreateStatic(waThread2, sizeof(waThread2),
                      NORMALPRIO + 10, Thread2, NULL);
}

RUN_AFTER(INIT_END) {
    usb_init();
}

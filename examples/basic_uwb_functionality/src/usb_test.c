#include <modules/worker_thread/worker_thread.h>

#include <ch.h>

#include <hal.h>

#include <shell.h>
#include <shell_cmd.h>
#include <chprintf.h>
#include <stdlib.h>     /* atoi */


#define WT hpwork_thread
WORKER_THREAD_DECLARE_EXTERN(WT)

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

    (void)arg;
    chRegSetThreadName("blinker1");
    while (true) {
        chprintf((BaseSequentialStream  *)&SD2, "plop [%d]\r\n", (unsigned long)chVTGetSystemTime());
        chThdSleepMilliseconds(1000);
    }
}

// handle request to change blink speed on green led
void cmd_blinkspeed(BaseSequentialStream *chp, int argc, char *argv[]) {

    (void)argv;
    uint16_t millis = 500;
    if (argc != 1) {
        chprintf(chp, "Usage: blinkspeed speed [ms]\r\n");
        return;
    }
    millis = atoi(argv[0]);
    chprintf(chp, "Got speed [%d]\r\n", millis);  // TODO : add UAVCAN Debug

}

static const ShellCommand commands[] = {
    {"blinkspeed", cmd_blinkspeed}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream  *)&SD2,
  commands
};

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

static THD_WORKING_AREA(waShellThread, 2048);
static THD_FUNCTION(ShellThread, arg) {
    (void)arg;
    chRegSetThreadName("shellthread");
    while (true) {
        thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                                "shell", NORMALPRIO + 1,
                                                shellThread, (void *)&shell_cfg1);
        chThdWait(shelltp);               /* Waiting termination.             */
        chThdSleepMilliseconds(1000);
    }
}

// Init the USB
void usb_init(void) {
    sdStart(&SD2, NULL);
    /*
    * Activates the USB driver and then the USB bus pull-up on D+.
    * Note, a delay is inserted in order to not have to disconnect the cable
    * after a reset.
    */
   // usbDisconnectBus(serusbcfg.usbp);
  //  chThdSleepMilliseconds(1500);
  //  usbStart(serusbcfg.usbp, &usbcfg);
  //  usbConnectBus(serusbcfg.usbp);
    /*
     * Shell manager initialization.
     */
    shellInit();
    chThdCreateStatic(waShellThread, sizeof(waShellThread), NORMALPRIO+1,
                      ShellThread, NULL);
    chThdCreateStatic(waThread1, sizeof(waThread1),
                      NORMALPRIO + 10, Thread1, NULL);

}

// Function call after the end of init
RUN_AFTER(INIT_END) {
    usb_init();
   //  Add new tasks to worker thread
   //  make led blink and report every 1000ms (repeated)
}

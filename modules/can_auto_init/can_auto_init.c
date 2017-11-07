#include <common/ctor.h>
#include <can/can.h>
#include <can/autobaud.h>
#include <timing/timing.h>

#ifdef MODULE_BOOTLOADER_COMPAT_ENABLED
#include <bootloader_compat/boot_msg.h>
#include <bootloader_compat/app_descriptor.h>
#endif

#define CANBUS_AUTOBAUD_SWITCH_INTERVAL_US 1000000

static struct can_autobaud_state_s autobaud_state;

RUN_BEFORE(OMD_UAVCAN_INIT) {
    uint32_t canbus_baud;
    bool canbus_autobaud_enable;

#ifdef MODULE_BOOTLOADER_COMPAT_ENABLED
    bool boot_msg_valid = get_boot_msg_valid();

    if (boot_msg_valid && can_autobaud_baudrate_valid(boot_msg.canbus_info.baudrate)) {
        canbus_baud = boot_msg.canbus_info.baudrate;
    } else if (can_autobaud_baudrate_valid(shared_get_parameters(&shared_app_descriptor)->canbus_baudrate)) {
        canbus_baud = shared_get_parameters(&shared_app_descriptor)->canbus_baudrate;
    } else {
        canbus_baud = 1000000;
    }

    if (boot_msg_valid && can_autobaud_baudrate_valid(boot_msg.canbus_info.baudrate)) {
        canbus_autobaud_enable = false;
    } else if (shared_get_parameters(&shared_app_descriptor)->canbus_disable_auto_baud) {
        canbus_autobaud_enable = false;
    } else {
        canbus_autobaud_enable = true;
    }
#else
    canbus_autobaud_enable = true;
    canbus_baud = 1000000;
#endif

    canbus_autobaud_enable = false;
    canbus_baud = 1000000;

    if (canbus_autobaud_enable) {
        can_autobaud_start(&autobaud_state, 0, canbus_baud, CANBUS_AUTOBAUD_SWITCH_INTERVAL_US);
        while (!autobaud_state.success) {
            canbus_baud = can_autobaud_update(&autobaud_state);
        }
    }
    can_init(0, canbus_baud, false);
}

#include <common/ctor.h>
#include <modules/can/can.h>
#include <modules/can/autobaud.h>
#include <modules/timing/timing.h>

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
#include <modules/app_descriptor/app_descriptor.h>
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
#include <modules/boot_msg/boot_msg.h>
#endif

#define CANBUS_AUTOBAUD_SWITCH_INTERVAL_US 1000000

static struct can_autobaud_state_s autobaud_state;

RUN_BEFORE(UAVCAN_INIT) {
    uint32_t canbus_baud = 1000000;
    bool canbus_autobaud_enable = true;

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
    if (can_autobaud_baudrate_valid(shared_get_parameters(&shared_app_descriptor)->canbus_baudrate)) {
        canbus_baud = shared_get_parameters(&shared_app_descriptor)->canbus_baudrate;
    }

    if (shared_get_parameters(&shared_app_descriptor)->canbus_disable_auto_baud) {
        canbus_autobaud_enable = false;
    }
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
    bool boot_msg_valid = get_boot_msg_valid();

    if (boot_msg_valid && can_autobaud_baudrate_valid(boot_msg.canbus_info.baudrate)) {
        canbus_baud = boot_msg.canbus_info.baudrate;
        canbus_autobaud_enable = false;
    }
#endif

    if (canbus_autobaud_enable) {
        can_autobaud_start(&autobaud_state, 0, canbus_baud, CANBUS_AUTOBAUD_SWITCH_INTERVAL_US);
        while (!autobaud_state.success) {
            canbus_baud = can_autobaud_update(&autobaud_state);
        }
    }
    can_init(0, canbus_baud, false);
}

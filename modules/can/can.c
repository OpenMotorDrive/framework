#include <common/ctor.h>

#ifdef MODULE_BOOTLOADER_COMPAT_ENABLED
#include <bootloader_compat/boot_msg.h>
#include <bootloader_compat/app_descriptor.h>
#endif

static const uint32_t valid_baudrates[] = {
    125000,
    250000,
    500000,
    1000000
};

static void canbus_init(uint32_t baud, bool silent) {
    baudrate = baud;
    successful_recv = false;

    const CANConfig cancfg = {
        CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
        (silent?CAN_BTR_SILM:0) | CAN_BTR_SJW(0) | CAN_BTR_TS2(2-1) |
        CAN_BTR_TS1(15-1) | CAN_BTR_BRP((STM32_PCLK1/18)/baudrate - 1)
    };

    canStart(&CAND1, &cancfg);
}


RUN_AFTER(BOOT_MSG_RETRIEVAL) {
    uint32_t canbus_baud;
    bool canbus_autobaud_enable;
#ifdef MODULE_BOOTLOADER_COMPAT_ENABLED
    bool boot_msg_valid = get_boot_msg_valid();

    if (boot_msg_valid && canbus_baudrate_valid(boot_msg.canbus_info.baudrate)) {
        canbus_baud = boot_msg.canbus_info.baudrate;
    } else if (canbus_baudrate_valid(shared_get_parameters(&shared_app_descriptor)->canbus_baudrate)) {
        canbus_baud = shared_get_parameters(&shared_app_descriptor)->canbus_baudrate;
    } else {
        canbus_baud = 1000000;
    }

    if (boot_msg_valid && canbus_baudrate_valid(boot_msg.canbus_info.baudrate)) {
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


}

#include "app_descriptor.h"
#include <app_config.h>

static const struct shared_app_parameters_s shared_app_parameters = {
    .boot_delay_sec = APP_CONFIG_BOOT_DELAY_SEC,
    .canbus_disable_auto_baud = !APP_CONFIG_CAN_AUTO_BAUD_ENABLE,
    .canbus_baudrate = APP_CONFIG_CAN_DEFAULT_BAUDRATE,
    .canbus_local_node_id = APP_CONFIG_CAN_LOCAL_NODE_ID
};

const struct shared_app_descriptor_s shared_app_descriptor __attribute__((section(".app_descriptor"),used)) = {
    .signature = SHARED_APP_DESCRIPTOR_SIGNATURE,
    .image_crc = 0,
    .image_size = 0,
    .vcs_commit = GIT_HASH,
    .major_version = 1,
    .minor_version = 0,
    .parameters_fmt = SHARED_APP_PARAMETERS_FMT,
    .parameters_ignore_crc64 = 1,
    .parameters = {&shared_app_parameters, 0}
};

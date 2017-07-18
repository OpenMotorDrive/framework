#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef SHARED_MSG_PACKED
#define SHARED_MSG_PACKED __attribute__((packed))
#endif

enum shared_msg_t {
    SHARED_MSG_BOOT = 0,
    SHARED_MSG_FIRMWAREUPDATE = 1,
    SHARED_MSG_BOOT_INFO = 2,
    SHARED_MSG_CANBUS_INFO = 3
};

struct shared_canbus_info_s {
    uint32_t baudrate;
    uint8_t local_node_id;
} SHARED_MSG_PACKED;

enum shared_hw_info_board_desc_fmt_t {
    SHARED_HW_INFO_BOARD_DESC_FMT_NONE = 0
};

enum shared_boot_reason_t {
    SHARED_BOOT_REASON_TIMEOUT = 0,
    SHARED_BOOT_REASON_REBOOT_COMMAND = 1,
    SHARED_BOOT_REASON_APPLICATION_COMMAND = 2,
    SHARED_BOOT_REASON_FIRMWARE_UPDATE = 3,
};

struct shared_hw_info_s {
    const char* const hw_name;
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
    uint8_t board_desc_fmt;
    const void* const board_desc;
} SHARED_MSG_PACKED;

struct shared_boot_msg_s {
    struct shared_canbus_info_s canbus_info;
    uint8_t boot_reason; // >= 127 are vendor/application-specific codes
}  SHARED_MSG_PACKED;

struct shared_firmwareupdate_msg_s {
    struct shared_canbus_info_s canbus_info;
    uint8_t source_node_id;
    char path[201];
} SHARED_MSG_PACKED;

struct shared_boot_info_msg_s {
    struct shared_canbus_info_s canbus_info;
    const struct shared_hw_info_s* hw_info;
    uint8_t boot_reason; // >= 127 are vendor/application-specific codes
} SHARED_MSG_PACKED;

union shared_msg_payload_u {
    struct shared_boot_msg_s boot_msg;
    struct shared_firmwareupdate_msg_s firmwareupdate_msg;
    struct shared_boot_info_msg_s boot_info_msg;
    struct shared_canbus_info_s canbus_info;
};

bool shared_msg_check_and_retreive(enum shared_msg_t* msgid, union shared_msg_payload_u* msg_payload);
void shared_msg_finalize_and_write(enum shared_msg_t msgid, const union shared_msg_payload_u* msg_payload);
void shared_msg_clear(void);

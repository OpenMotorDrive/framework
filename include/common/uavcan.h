#pragma once

#include <stdint.h>
#include <stdbool.h>

enum uavcan_loglevel_t {
    UAVCAN_LOGLEVEL_DEBUG = 0,
    UAVCAN_LOGLEVEL_INFO = 1,
    UAVCAN_LOGLEVEL_WARNING = 2,
    UAVCAN_LOGLEVEL_ERROR = 3,
};

enum uavcan_beginfirmwareupdate_error_t {
    UAVCAN_BEGINFIRMWAREUPDATE_ERROR_OK = 0,
    UAVCAN_BEGINFIRMWAREUPDATE_ERROR_INVALID_MODE = 1,
    UAVCAN_BEGINFIRMWAREUPDATE_ERROR_IN_PROGRESS = 2,
    UAVCAN_BEGINFIRMWAREUPDATE_ERROR_UNKNOWN = 255,
};

enum uavcan_node_mode_t {
    UAVCAN_MODE_OPERATIONAL = 0,
    UAVCAN_MODE_INITIALIZATION = 1,
    UAVCAN_MODE_MAINTENANCE = 2,
    UAVCAN_MODE_SOFTWARE_UPDATE = 3,
    UAVCAN_MODE_OFFLINE = 7,
};

enum uavcan_node_health_t {
    UAVCAN_HEALTH_OK = 0,
    UAVCAN_HEALTH_WARNING,
    UAVCAN_HEALTH_ERROR,
    UAVCAN_HEALTH_CRITICAL
};

struct uavcan_transfer_info_s {
    void* canardInstance;
    uint8_t remote_node_id;
    uint8_t transfer_id;
    uint8_t priority;
};

struct uavcan_node_info_s {
    const char* hw_name;
    uint8_t hw_major_version;
    uint8_t hw_minor_version;
    uint8_t sw_major_version;
    uint8_t sw_minor_version;
    bool sw_vcs_commit_available;
    uint32_t sw_vcs_commit;
    bool sw_image_crc_available;
    uint64_t sw_image_crc;
};

typedef void (*restart_handler_ptr)(struct uavcan_transfer_info_s transfer_info, uint64_t magic);
typedef void (*file_beginfirmwareupdate_handler_ptr)(struct uavcan_transfer_info_s transfer_info, uint8_t source_node_id, const char* path);
typedef void (*file_read_response_handler_ptr)(uint8_t transfer_id, int16_t error, const uint8_t* data, uint16_t data_len, bool eof);
typedef void (*uavcan_ready_handler_ptr)(void);

void uavcan_init(void);
void uavcan_update(void);
void uavcan_set_uavcan_ready_cb(uavcan_ready_handler_ptr cb);
void uavcan_set_restart_cb(restart_handler_ptr cb);
void uavcan_set_file_beginfirmwareupdate_cb(file_beginfirmwareupdate_handler_ptr cb);
void uavcan_set_file_read_response_cb(file_read_response_handler_ptr cb);
void uavcan_set_node_mode(enum uavcan_node_mode_t mode);
void uavcan_set_node_health(enum uavcan_node_health_t health);
void uavcan_set_node_id(uint8_t node_id);
uint8_t uavcan_get_node_id(void);
void uavcan_set_node_info(struct uavcan_node_info_s new_node_info);

void uavcan_send_debug_key_value(const char* name, float val);
void uavcan_send_debug_logmessage(enum uavcan_loglevel_t log_level, const char* source, const char* text);
void uavcan_send_file_beginfirmwareupdate_response(struct uavcan_transfer_info_s* transfer_info, enum uavcan_beginfirmwareupdate_error_t error, const char* error_message);
void uavcan_send_restart_response(struct uavcan_transfer_info_s* transfer_info, bool ok);
uint8_t uavcan_send_file_read_request(uint8_t remote_node_id, const uint64_t offset, const char* path);

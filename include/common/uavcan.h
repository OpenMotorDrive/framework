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


enum uavcan_param_value_union_type_t {
    UAVCAN_PARAM_VALUE_TYPE_EMPTY,
    UAVCAN_PARAM_VALUE_TYPE_INT64,
    UAVCAN_PARAM_VALUE_TYPE_FLOAT32,
    UAVCAN_PARAM_VALUE_TYPE_BOOL,
    UAVCAN_PARAM_VALUE_TYPE_STRING
};

struct uavcan_param_value_s {
    uint8_t type;
    union {
        int64_t integer_value;
        float real_value;
        uint8_t boolean_value;
        struct {
            uint8_t string_value_len;
            uint8_t string_value[128];
        };
    };
};

enum uavcan_param_numericvalue_union_type_t {
    UAVCAN_PARAM_NUMERICVALUE_TYPE_EMPTY,
    UAVCAN_PARAM_NUMERICVALUE_TYPE_INT64,
    UAVCAN_PARAM_NUMERICVALUE_TYPE_FLOAT32
};

struct uavcan_param_numericvalue_s {
    uint8_t type;
    union {
        int64_t integer_value;
        float real_value;
    };
};

struct uavcan_param_getset_request_s {
    uint16_t index;
    struct uavcan_param_value_s value;
    uint8_t name_len;
    uint8_t name[92];
};

struct uavcan_param_getset_response_s {
    struct uavcan_param_value_s value;
    struct uavcan_param_value_s default_value;
    struct uavcan_param_numericvalue_s max_value;
    struct uavcan_param_numericvalue_s min_value;
    uint8_t name_len;
    uint8_t name[92];
};

enum uavcan_param_executeopcode_opcode_t {
    UAVCAN_PARAM_EXECUTEOPCODE_OPCODE_SAVE = 0,
    UAVCAN_PARAM_EXECUTEOPCODE_OPCODE_ERASE
};

struct uavcan_param_executeopcode_request_s {
    uint8_t opcode;
    int64_t argument;
};

struct uavcan_param_executeopcode_response_s {
    int64_t argument;
    bool ok;
};

typedef void (*restart_handler_ptr)(struct uavcan_transfer_info_s transfer_info, uint64_t magic);
typedef void (*file_beginfirmwareupdate_handler_ptr)(struct uavcan_transfer_info_s transfer_info, uint8_t source_node_id, const char* path);
typedef void (*param_getset_request_handler_ptr)(struct uavcan_transfer_info_s transfer_info, struct uavcan_param_getset_request_s* request);
typedef void (*param_executeopcode_request_handler_ptr)(struct uavcan_transfer_info_s transfer_info, struct uavcan_param_executeopcode_request_s* request);
typedef void (*file_read_response_handler_ptr)(uint8_t transfer_id, int16_t error, const uint8_t* data, uint16_t data_len, bool eof);
typedef void (*uavcan_ready_handler_ptr)(void);

void uavcan_acquire(void);
void uavcan_release(void);
void uavcan_init(void);
void uavcan_update(void);
void uavcan_set_uavcan_ready_cb(uavcan_ready_handler_ptr cb);
void uavcan_set_restart_cb(restart_handler_ptr cb);
void uavcan_set_file_beginfirmwareupdate_cb(file_beginfirmwareupdate_handler_ptr cb);
void uavcan_set_file_read_response_cb(file_read_response_handler_ptr cb);
void uavcan_set_param_getset_request_cb(param_getset_request_handler_ptr cb);
void uavcan_set_param_executeopcode_request_cb(param_executeopcode_request_handler_ptr cb);
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

void uavcan_send_param_getset_response(struct uavcan_transfer_info_s* transfer_info, const struct uavcan_param_getset_response_s* response);

void uavcan_send_param_executeopcode_response(struct uavcan_transfer_info_s* transfer_info, const struct uavcan_param_executeopcode_response_s* response);

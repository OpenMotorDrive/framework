#pragma once

#include <stdint.h>

#ifndef APP_DESCRIPTOR_ALIGNED_AND_PACKED
#define APP_DESCRIPTOR_ALIGNED_AND_PACKED __attribute__((aligned(8),packed))
#endif

#ifndef APP_DESCRIPTOR_PACKED
#define APP_DESCRIPTOR_PACKED __attribute__((packed))
#endif

#define SHARED_APP_DESCRIPTOR_SIGNATURE "\x40\xa2\xe4\xf1\x64\x68\x91\x06"

#define SHARED_APP_PARAMETERS_FMT 1

struct shared_app_parameters_s {
    // this index is incremented on every param write - if two param structure pointers are provided,
    // use a signed integer comparison to determine the most recently written structure
    uint8_t param_idx;
    uint8_t boot_delay_sec;
    uint32_t canbus_disable_auto_baud : 1;
    uint32_t canbus_baudrate : 31;
    uint8_t canbus_local_node_id;
    uint64_t crc64;
} APP_DESCRIPTOR_PACKED;

struct shared_app_descriptor_s {
    char signature[8];
    uint64_t image_crc;
    uint32_t image_size;
    uint32_t vcs_commit;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t parameters_fmt:7;
    uint8_t parameters_ignore_crc64:1;
    const struct shared_app_parameters_s* parameters[2];
} APP_DESCRIPTOR_ALIGNED_AND_PACKED;

const struct shared_app_descriptor_s* shared_find_app_descriptor(uint8_t* buf, uint32_t buf_len);
const struct shared_app_parameters_s* shared_get_parameters(const struct shared_app_descriptor_s* descriptor);

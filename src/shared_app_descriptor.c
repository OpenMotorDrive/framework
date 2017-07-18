#include <common/shared_app_descriptor.h>
#include <common/crc64_we.h>
#include <stdbool.h>
#include <string.h>

static const void* shared_find_marker(uint64_t marker, uint8_t* buf, uint32_t buf_len)
{
    for (uint32_t i=0; i<buf_len-sizeof(marker); i++) {
        if (!memcmp(&buf[i], &marker, sizeof(marker))) {
            return &buf[i];
        }
    }
    return 0;
}

const struct shared_app_descriptor_s* shared_find_app_descriptor(uint8_t* buf, uint32_t buf_len)
{
    return shared_find_marker(*((uint64_t*)SHARED_APP_DESCRIPTOR_SIGNATURE), buf, buf_len);
}

static bool param_struct_valid(const struct shared_app_parameters_s* parameters)
{
    return parameters && crc64_we((uint8_t*)parameters, sizeof(struct shared_app_parameters_s)-sizeof(uint64_t), 0) == parameters->crc64;
}

const struct shared_app_parameters_s* shared_get_parameters(const struct shared_app_descriptor_s* descriptor)
{
    if (descriptor->parameters_fmt != SHARED_APP_PARAMETERS_FMT) {
        return 0;
    }

    const struct shared_app_parameters_s* ret = 0;

    for (uint8_t i=0; i<2; i++) {
        if (descriptor->parameters_ignore_crc64 || param_struct_valid(descriptor->parameters[i])) {
            if (!ret || (int8_t)(descriptor->parameters[i]->param_idx) > (int8_t)(ret->param_idx)) {
                ret = descriptor->parameters[i];
            }
        }
    }

    return ret;
}

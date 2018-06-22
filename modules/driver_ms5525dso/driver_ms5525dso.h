#pragma once

#include <modules/spi_device/spi_device.h>

struct ms5525dso_instance_s {
    struct spi_device_s spi_dev;
    uint16_t C_coeff[8];
};

bool ms5525dso_init(struct ms5525dso_instance_s* instance, uint8_t spi_idx, uint32_t select_line);
bool ms5525dso_get_measurement(struct ms5525dso_instance_s* instance, float* ret);

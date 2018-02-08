#pragma once

#include <modules/spi_device/spi_device.h>

enum invensense_imu_type_t {
    INVENSENSE_IMU_TYPE_MPU6000,
    INVENSENSE_IMU_TYPE_MPU6500,
    INVENSENSE_IMU_TYPE_MPU9250,
    INVENSENSE_IMU_TYPE_MPU9255,
    INVENSENSE_IMU_TYPE_ICM20608,
    INVENSENSE_IMU_TYPE_ICM20602,
};

struct invensense_instance_s {
    struct spi_device_s spi_dev;
    enum invensense_imu_type_t imu_type;
};

bool invensense_init(struct invensense_instance_s* instance, uint8_t spi_idx, uint32_t select_line, enum invensense_imu_type_t imu_type);
size_t invensense_get_fifo_count(struct invensense_instance_s* instance);
size_t invensense_read_fifo(struct invensense_instance_s* instance, void* buf);

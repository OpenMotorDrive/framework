#pragma once

#include <modules/spi_device/spi_device.h>

enum icm20x48_imu_type_t {
    ICM20x48_IMU_TYPE_ICM20948,
};

struct icm20x48_instance_s {
    struct spi_device_s spi_dev;
    enum icm20x48_imu_type_t imu_type;
    uint8_t curr_bank;
};

bool icm20x48_init(struct icm20x48_instance_s* instance, uint8_t spi_idx, uint32_t select_line, enum icm20x48_imu_type_t imu_type);
void icm20x48_i2c_slv_read_enable(struct icm20x48_instance_s* instance);
bool icm20x48_i2c_slv_set_passthrough(struct icm20x48_instance_s* instance, uint8_t slv_id, uint8_t addr, uint8_t reg, uint8_t size);
bool icm20x48_i2c_slv_read(struct icm20x48_instance_s* instance, uint8_t address, uint8_t reg, uint8_t* ret);
bool icm20x48_i2c_slv_write(struct icm20x48_instance_s* instance, uint8_t address, uint8_t reg, uint8_t value);
uint8_t icm20x48_read_reg(struct icm20x48_instance_s* instance, uint16_t reg);
void icm20x48_write_reg(struct icm20x48_instance_s* instance, uint16_t reg, uint8_t value);

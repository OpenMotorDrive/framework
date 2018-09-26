#include "driver_icm20x48.h"
#include "icm20x48_internal.h"
#include <common/bswap.h>
#include <modules/timing/timing.h>
#ifdef MODULE_UAVCAN_DEBUG_ENABLED
#include <modules/uavcan_debug/uavcan_debug.h>
#define ICM_DEBUG(...) uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_DEBUG, "ICM", __VA_ARGS__)
#else
#define ICM_DEBUG(...) {}
#endif

static uint8_t icm20x48_get_whoami(enum icm20x48_imu_type_t imu_type);

bool icm20x48_init(struct icm20x48_instance_s* instance, uint8_t spi_idx, uint32_t select_line, enum icm20x48_imu_type_t imu_type) {
    // Ensure sufficient power-up time has elapsed
    chThdSleep(MS2ST(100));

    instance->curr_bank = 99;

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 8000000, 16, SPI_DEVICE_FLAG_CPHA|SPI_DEVICE_FLAG_CPOL);

    if (icm20x48_read_reg(instance, ICM20948_REG_WHO_AM_I) != icm20x48_get_whoami(imu_type)) {
        return false;
    }

    // Read USER_CTRL, disable MST_I2C, write USER_CTRL, and wait long enough for any active I2C transaction to complete
    icm20x48_write_reg(instance, ICM20948_REG_USER_CTRL,  icm20x48_read_reg(instance, ICM20948_REG_USER_CTRL) & ~(1<<5));
    chThdSleep(MS2ST(10));
    // Perform a device reset, wait for completion, then wake the device
    // Datasheet is unclear on time required for wait time after reset, but mentions 100ms under "start-up time for register read/write from power-up"
    icm20x48_write_reg(instance, ICM20948_REG_PWR_MGMT_1, 1<<7);
    usleep(10000);

    icm20x48_write_reg(instance, ICM20948_REG_PWR_MGMT_1, 1);
    // Wait for reset to complete
    {
        uint32_t tbegin = chVTGetSystemTimeX();
        while (icm20x48_read_reg(instance, ICM20948_REG_PWR_MGMT_1) & 1<<7) {
            uint32_t tnow = chVTGetSystemTimeX();
            if (tnow-tbegin > MS2ST(100)) {
                return false;
            }
        }
    }

    return true;
}

void icm20x48_i2c_slv_read_enable(struct icm20x48_instance_s* instance)
{
    // Disable the I2C slave interface (SPI-only) and enable the I2C master interface to the AK09916
    icm20x48_write_reg(instance, ICM20948_REG_USER_CTRL, (1<<4)|(1<<5));

    // Set up the I2C master clock as recommended in the datasheet
    icm20x48_write_reg(instance, ICM20948_REG_I2C_MST_CTRL, 7);

    // Configure the I2C master to enable access at sample rate specified in SLV4
    icm20x48_write_reg(instance, ICM20948_REG_I2C_MST_DELAY_CTRL, 1<<7);
}

bool icm20x48_i2c_slv_set_passthrough(struct icm20x48_instance_s* instance, uint8_t slv_id, uint8_t addr, uint8_t reg, uint8_t size)
{
    if (size > 15) {
        return false;
    }
    //icm20x48_write_reg(instance, ICM20948_REG_I2C_MST_STATUS, 0);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV0_ADDR + 4*slv_id, addr);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV0_REG  + 4*slv_id, reg);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV0_CTRL + 4*slv_id, (1<<7)|size);
    return true;
}

static void icm20x48_select_bank(struct icm20x48_instance_s* instance, uint8_t bank) {
    uint16_t data = (ICM20948_REG_BANK_SEL << 8) | (bank<<4);
    instance->curr_bank = bank;
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, 1, &data);
    spi_device_end(&instance->spi_dev);
}

uint8_t icm20x48_read_reg(struct icm20x48_instance_s* instance, uint16_t reg){

    uint16_t _reg = (((uint16_t)(GET_REG(reg) | 0x80)) << 8);
    uint16_t ret = 0;
    uint8_t _bank = GET_BANK(reg);
    if (_bank != instance->curr_bank) {
        icm20x48_select_bank(instance, _bank);
    }
    spi_device_begin(&instance->spi_dev);
    spi_device_exchange(&instance->spi_dev, 1, &_reg, &ret);
    spi_device_end(&instance->spi_dev);
    return ret;
}

void icm20x48_write_reg(struct icm20x48_instance_s* instance, uint16_t reg, uint8_t value) {
    uint8_t _bank = GET_BANK(reg);
    uint16_t data = (((uint16_t)GET_REG(reg)) << 8) | value;
    if (_bank != instance->curr_bank) {
        icm20x48_select_bank(instance, _bank);
    }
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, 1 , &data);
    spi_device_end(&instance->spi_dev);
}

static uint8_t icm20x48_get_whoami(enum icm20x48_imu_type_t imu_type) {
    switch(imu_type) {
        case ICM20x48_IMU_TYPE_ICM20948:
            return 0xEA;
    }
    return 0;
}

static bool icm20x48_i2c_slv_wait_for_completion(struct icm20x48_instance_s* instance, uint32_t timeout_us) {
    uint32_t tbegin_us = micros();
    while(!(icm20x48_read_reg(instance, ICM20948_REG_I2C_MST_STATUS) & (1<<6))) {
        if (micros() - tbegin_us > timeout_us) {
            return false;
        }
    }
    return true;
}

bool icm20x48_i2c_slv_read(struct icm20x48_instance_s* instance, uint8_t address, uint8_t reg, uint8_t* ret) {
    icm20x48_write_reg(instance, ICM20948_REG_I2C_MST_STATUS, 0);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_ADDR, address|0x80);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_REG, reg);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_CTRL, (1<<7)|(1<<6));
    if (!icm20x48_i2c_slv_wait_for_completion(instance, 10000)) {
        return false;
    }
    *ret = icm20x48_read_reg(instance, ICM20948_REG_I2C_SLV4_DI);
    return true;
}

bool icm20x48_i2c_slv_write(struct icm20x48_instance_s* instance, uint8_t address, uint8_t reg, uint8_t value) {
    icm20x48_write_reg(instance, ICM20948_REG_I2C_MST_STATUS, 0);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_ADDR, address);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_REG, reg);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_DO, value);
    icm20x48_write_reg(instance, ICM20948_REG_I2C_SLV4_CTRL, (1<<7)|(1<<6));
    if (!icm20x48_i2c_slv_wait_for_completion(instance, 10000)) {
        return false;
    }
    return true;
}


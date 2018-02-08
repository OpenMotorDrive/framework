#include "driver_invensense.h"
#include "invensense_internal.h"
#include <common/bswap.h>

#ifdef MODULE_UAVCAN_DEBUG_ENABLED
#include <modules/uavcan_debug/uavcan_debug.h>
#define ICM_DEBUG(...) uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_DEBUG, "ICM", __VA_ARGS__)
#else
#define ICM_DEBUG(...) {}
#endif

static uint8_t invensense_read8(struct invensense_instance_s* instance, uint8_t reg);
static uint16_t invensense_read16(struct invensense_instance_s* instance, uint8_t reg);
static void invensense_write8(struct invensense_instance_s* instance, uint8_t reg, uint8_t value);
static void invensense_read(struct invensense_instance_s* instance, uint8_t reg, size_t len, void* buf);
static void invensense_write(struct invensense_instance_s* instance, uint8_t reg, size_t len, void* buf);
static uint8_t invensense_get_whoami(enum invensense_imu_type_t imu_type);

bool invensense_init(struct invensense_instance_s* instance, uint8_t spi_idx, uint32_t select_line, enum invensense_imu_type_t imu_type) {
    // Ensure sufficient power-up time has elapsed
    chThdSleep(MS2ST(100));

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 10000, 8, SPI_DEVICE_FLAG_CPHA|SPI_DEVICE_FLAG_CPOL);

    if (invensense_read8(instance, INVENSENSE_REG_WHO_AM_I) != invensense_get_whoami(imu_type)) {
        return false;
    }

    spi_device_set_max_speed_hz(&instance->spi_dev, 10000000);

    // Reset device
    invensense_write8(instance, INVENSENSE_REG_PWR_MGMT_1, 1<<7);

    // Wait for reset to complete
    {
        uint32_t tbegin = chVTGetSystemTimeX();
        while (invensense_read8(instance, INVENSENSE_REG_PWR_MGMT_1) & 1<<7) {
            uint32_t tnow = chVTGetSystemTimeX();
            if (tnow-tbegin > MS2ST(100)) {
                return false;
            }
        }
    }

    chThdSleep(MS2ST(10));

    // Reset value of CONFIG is 0x80 - datasheet instructs us to clear bit 7
    invensense_write8(instance, INVENSENSE_REG_CONFIG, 0);

    // Reset value of PWR_MGMT_1 is 0x41 - clear sleep bit
    invensense_write8(instance, INVENSENSE_REG_PWR_MGMT_1, 1);

    // Configure gyro for 2000 dps FSR and max sample rate
    invensense_write8(instance, INVENSENSE_REG_GYRO_CONFIG, 0x18 | 0x01);

    // Configure accelerometer for 16 G FSR
    invensense_write8(instance, INVENSENSE_REG_ACCEL_CONFIG, 0x18);

    // Configure accelerometer for max sample rate
    invensense_write8(instance, INVENSENSE_REG_ACCEL_CONFIG2, 1<<3);

    // Enable and reset FIFO
    invensense_write8(instance, INVENSENSE_REG_FIFO_EN, 0x18);
    invensense_write8(instance, INVENSENSE_REG_USER_CTRL, 1<<6 | 1<<2);
    return true;
}

size_t invensense_get_fifo_count(struct invensense_instance_s* instance) {
    return invensense_read16(instance, INVENSENSE_REG_FIFO_COUNTH);
}

size_t invensense_read_fifo(struct invensense_instance_s* instance, void* buf) {
    size_t count = invensense_get_fifo_count(instance);
    invensense_read(instance, INVENSENSE_REG_FIFO_R_W, count, buf);
    for (size_t i=0; i<count/2; i++) {
        ((uint16_t*)buf)[i] = be16_to_cpu(((uint16_t*)buf)[i]);
    }
    return count;
}

static uint8_t invensense_read8(struct invensense_instance_s* instance, uint8_t reg) {
    uint8_t ret;
    invensense_read(instance, reg, sizeof(ret), &ret);
    return ret;
}

static uint16_t invensense_read16(struct invensense_instance_s* instance, uint8_t reg) {
    uint16_t ret;
    invensense_read(instance, reg, sizeof(ret), &ret);
    return be16_to_cpu(ret);
}

static void invensense_write8(struct invensense_instance_s* instance, uint8_t reg, uint8_t value) {
    invensense_write(instance, reg, sizeof(value), &value);
}

static void invensense_read(struct invensense_instance_s* instance, uint8_t reg, size_t len, void* buf) {
    reg |= 0x80;

    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(reg), &reg);
    spi_device_receive(&instance->spi_dev, len, buf);
    spi_device_end(&instance->spi_dev);
}

static void invensense_write(struct invensense_instance_s* instance, uint8_t reg, size_t len, void* buf) {
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(reg), &reg);
    spi_device_send(&instance->spi_dev, len, buf);
    spi_device_end(&instance->spi_dev);
}

static uint8_t invensense_get_whoami(enum invensense_imu_type_t imu_type) {
    switch(imu_type) {
        case INVENSENSE_IMU_TYPE_MPU6000:
            return 0x68;
        case INVENSENSE_IMU_TYPE_MPU6500:
            return 0x70;
        case INVENSENSE_IMU_TYPE_MPU9250:
            return 0x71;
        case INVENSENSE_IMU_TYPE_MPU9255:
            return 0x73;
        case INVENSENSE_IMU_TYPE_ICM20608:
            return 0xaf;
        case INVENSENSE_IMU_TYPE_ICM20602:
            return 0x12;
    }
    return 0;
}

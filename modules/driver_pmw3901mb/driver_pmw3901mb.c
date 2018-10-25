#include "driver_pmw3901mb.h"
#include "pmw3901mb_internal.h"
#include <common/bswap.h>
#include <modules/uavcan_debug/uavcan_debug.h>

bool pmw3901mb_burst_read(struct pmw3901mb_instance_s* instance, struct pmw3901mb_motion_report_s* ret) {
    spi_device_begin(&instance->spi_dev);
    const uint8_t motion_burst = PMW3901MB_MOTION_BURST;
    spi_device_send(&instance->spi_dev, 1, &motion_burst);
    chThdSleepMicroseconds(PMW3901MB_TSRAD_US);
    spi_device_receive(&instance->spi_dev, sizeof(*ret), ret);
    spi_device_end(&instance->spi_dev);
    chThdSleepMicroseconds(PMW3901MB_TSR_US);
    
    ret->delta_x = le16_to_cpu(ret->delta_x);
    ret->delta_y = le16_to_cpu(ret->delta_y);
    
    return ret->motion & 0x80;
}

bool pmw3901mb_motion_detected(struct pmw3901mb_instance_s* instance)
{
    bool motion_detected = false;
    
    uint8_t motion_reg = pmw3901mb_read(instance, PMW3901MB_MOTION);
    if (motion_reg & 0x80)
        motion_detected = true;
    
    return motion_detected;
}

int16_t pmw3901mb_read_dx(struct pmw3901mb_instance_s* instance)
{
    int16_t dx = 0;
    uint8_t value = 0;
    
    value = pmw3901mb_read(instance, PMW3901MB_DELTA_X_H);
    dx = (uint16_t)value << 8;
    dx += pmw3901mb_read(instance, PMW3901MB_DELTA_X_L);
    
    return dx;
}

int16_t pmw3901mb_read_dy(struct pmw3901mb_instance_s* instance)
{
    int16_t dy = 0;
    uint8_t value = 0;
    
    value = pmw3901mb_read(instance, PMW3901MB_DELTA_Y_H);
    dy = (uint16_t)value << 8;
    dy += pmw3901mb_read(instance, PMW3901MB_DELTA_Y_L);
    
    return dy;
}

void pmw3901mb_write(struct pmw3901mb_instance_s* instance, uint8_t reg, uint8_t value)
{
    reg |= 0x80;
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, 1, &reg);
    spi_device_send(&instance->spi_dev, 1, &value);
    spi_device_end(&instance->spi_dev);
    chThdSleepMicroseconds(PMW3901MB_TSWW_US);
}

uint8_t pmw3901mb_read(struct pmw3901mb_instance_s* instance, uint8_t reg)
{
    uint8_t value = 0;
    
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, 1, &reg);
    chThdSleepMicroseconds(PMW3901MB_TSRAD_US);
    spi_device_receive(&instance->spi_dev, 1, &value);
    spi_device_end(&instance->spi_dev);
    chThdSleepMicroseconds(PMW3901MB_TSR_US);
    
    return value;
}

bool pmw3901mb_init(struct pmw3901mb_instance_s* instance, uint8_t spi_idx, uint32_t select_line, enum pmw3901mb_type_t pmw3901mb_type) {
       
    if (pmw3901mb_type != PMW3901MB_TYPE_V1)
        return false;

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 2000, 8, SPI_DEVICE_FLAG_CPHA|SPI_DEVICE_FLAG_CPOL);
    spi_device_set_max_speed_hz(&instance->spi_dev, 2000000);

    chThdSleepMilliseconds(PMW3901MB_WAKEUP_MS);
    pmw3901mb_write(instance, PMW3901MB_POWER_UP_RESET, 0x5A);
    chThdSleepMilliseconds(1);
    
    // one-time read of registers per datasheet
    pmw3901mb_read(instance, PMW3901MB_MOTION);
    pmw3901mb_read(instance, PMW3901MB_DELTA_X_L);
    pmw3901mb_read(instance, PMW3901MB_DELTA_X_H);
    pmw3901mb_read(instance, PMW3901MB_DELTA_Y_L);
    pmw3901mb_read(instance, PMW3901MB_DELTA_Y_H);
    
    // proprietary optimizations per datasheet
    pmw3901mb_write(instance, 0x7F, 0x00);
    pmw3901mb_write(instance, 0x61, 0xAD);
    pmw3901mb_write(instance, 0x7F, 0x03);
    pmw3901mb_write(instance, 0x40, 0x00);
    pmw3901mb_write(instance, 0x7F, 0x05);
    pmw3901mb_write(instance, 0x41, 0xB3);
    pmw3901mb_write(instance, 0x43, 0xF1);
    pmw3901mb_write(instance, 0x45, 0x14);
    pmw3901mb_write(instance, 0x5B, 0x32);
    pmw3901mb_write(instance, 0x5F, 0x34);
    pmw3901mb_write(instance, 0x7B, 0x08);
    pmw3901mb_write(instance, 0x7F, 0x06);
    pmw3901mb_write(instance, 0x44, 0x1B);
    pmw3901mb_write(instance, 0x40, 0xBF);
    pmw3901mb_write(instance, 0x4E, 0x3F);
    pmw3901mb_write(instance, 0x7F, 0x08);
    pmw3901mb_write(instance, 0x65, 0x20);
    pmw3901mb_write(instance, 0x6A, 0x18);
    pmw3901mb_write(instance, 0x7F, 0x09);
    pmw3901mb_write(instance, 0x4F, 0xAF);
    pmw3901mb_write(instance, 0x5F, 0x40);
    pmw3901mb_write(instance, 0x48, 0x80);
    pmw3901mb_write(instance, 0x49, 0x80);
    pmw3901mb_write(instance, 0x57, 0x77);
    pmw3901mb_write(instance, 0x60, 0x78);
    pmw3901mb_write(instance, 0x61, 0x78);
    pmw3901mb_write(instance, 0x62, 0x08);
    pmw3901mb_write(instance, 0x63, 0x50);
    pmw3901mb_write(instance, 0x7F, 0x0A);
    pmw3901mb_write(instance, 0x45, 0x60);
    pmw3901mb_write(instance, 0x7F, 0x00);
    pmw3901mb_write(instance, 0x4D, 0x11);
    pmw3901mb_write(instance, 0x55, 0x80);
    pmw3901mb_write(instance, 0x74, 0x1F);
    pmw3901mb_write(instance, 0x75, 0x1F);
    pmw3901mb_write(instance, 0x4A, 0x78);
    pmw3901mb_write(instance, 0x4B, 0x78);
    pmw3901mb_write(instance, 0x44, 0x08);
    pmw3901mb_write(instance, 0x45, 0x50);
    pmw3901mb_write(instance, 0x64, 0xFF);
    pmw3901mb_write(instance, 0x65, 0x1F);
    pmw3901mb_write(instance, 0x7F, 0x14);
    pmw3901mb_write(instance, 0x65, 0x67);  // 0x65, 0x60 in data sheet ??
    pmw3901mb_write(instance, 0x66, 0x08);
    pmw3901mb_write(instance, 0x63, 0x70);  // 0x63, 0x78
    pmw3901mb_write(instance, 0x7F, 0x15);
    pmw3901mb_write(instance, 0x48, 0x48);  // 0x48, 0x58
    pmw3901mb_write(instance, 0x7F, 0x07);
    pmw3901mb_write(instance, 0x41, 0x0D);
    pmw3901mb_write(instance, 0x43, 0x14);
    pmw3901mb_write(instance, 0x4B, 0x0E);
    pmw3901mb_write(instance, 0x45, 0x0F);
    pmw3901mb_write(instance, 0x44, 0x42);
    pmw3901mb_write(instance, 0x4C, 0x80);
    pmw3901mb_write(instance, 0x7F, 0x10);
    pmw3901mb_write(instance, 0x5B, 0x02);
    pmw3901mb_write(instance, 0x7F, 0x07);
    pmw3901mb_write(instance, 0x40, 0x41);
    pmw3901mb_write(instance, 0x70, 0x00);
    
    chThdSleepMilliseconds(10);
    
    pmw3901mb_write(instance, 0x32, 0x44);
    pmw3901mb_write(instance, 0x7F, 0x07);
    pmw3901mb_write(instance, 0x40, 0x40);
    pmw3901mb_write(instance, 0x7F, 0x06);
    pmw3901mb_write(instance, 0x62, 0xF0);
    pmw3901mb_write(instance, 0x63, 0x00);
    pmw3901mb_write(instance, 0x7F, 0x0D);
    pmw3901mb_write(instance, 0x48, 0xC0);
    pmw3901mb_write(instance, 0x6F, 0xD5);
    pmw3901mb_write(instance, 0x7F, 0x00);
    pmw3901mb_write(instance, 0x5B, 0xA0);
    pmw3901mb_write(instance, 0x4E, 0xA8);
    pmw3901mb_write(instance, 0x5A, 0x50);
    pmw3901mb_write(instance, 0x40, 0x80);
    
    return true;
}

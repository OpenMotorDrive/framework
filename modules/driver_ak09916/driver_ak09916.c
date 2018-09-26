#include <modules/driver_icm20x48/icm20x48_internal.h>
#include "driver_ak09916.h"
#include <common/helpers.h>
#include <modules/timing/timing.h>
#pragma GCC optimize ("O0")

#define AK09916_I2C_ADDR 0x0C

struct compass_register_s {
    const uint8_t address;
    const bool writeable;
    const bool read_locks_data;
    const int8_t next_address;
    uint8_t value;
};

static struct compass_register_s compass_registers[] = {
    {0x00, false, false, 0x01, 0x48},
    {0x01, false, false, 0x02, 0x09},
    {0x02, false, false, 0x03, 0x00},
    {0x03, false, false, 0x10, 0x00},
    {0x10, false, false, 0x11, 0x00},
    {0x11, false,  true, 0x12, 0x00},
    {0x12, false,  true, 0x13, 0x00},
    {0x13, false,  true, 0x14, 0x00},
    {0x14, false,  true, 0x15, 0x00},
    {0x15, false,  true, 0x16, 0x00},
    {0x16, false,  true, 0x17, 0x00},
    {0x17, false,  true, 0x18, 0x00},
    {0x18, false, false, 0x00, 0x00},
    {0x30,  true, false, 0x31, 0x00},
    {0x31,  true, false, 0x32, 0x00},
    {0x32,  true, false, 0x30, 0x00},
};

static bool compass_update_lock;
static struct compass_register_s* compass_curr_register = &compass_registers[0];
static uint8_t compass_new_data_counter;

static struct compass_register_s* get_compass_register(uint8_t address) {
    for(uint8_t i=0; i<LEN(compass_registers); i++) {
        if (compass_registers[i].address == address) {
            return &compass_registers[i];
        }
    }
    return 0;
}

static void set_emulated_register(uint8_t address) {
    struct compass_register_s* next_compass_register = get_compass_register(address);
    if (next_compass_register) {
        compass_curr_register = next_compass_register;
    }
}

void ak09916_recv_byte(uint8_t recv_byte_idx, uint8_t recv_byte) {
    if (recv_byte_idx == 0) {
        set_emulated_register(recv_byte);
    } else {
        if (compass_curr_register->writeable) {
            compass_curr_register->value = recv_byte;
        }

        set_emulated_register(compass_curr_register->next_address);
    }
}

uint8_t ak09916_send_byte() {
    struct compass_register_s* st1_reg = get_compass_register(0x10);
    struct compass_register_s* accessed_reg = compass_curr_register;

    if (accessed_reg->read_locks_data && (st1_reg->value&(1<<1)) != 0) {
        compass_update_lock = true;
        st1_reg->value &= ~0b11;
    }

    if (accessed_reg->address == 0x18) {
        compass_update_lock = false;
    }

    set_emulated_register(accessed_reg->next_address);

    return accessed_reg->value;
}

// for now, it is assumed that this is called at 100hz
static void i2c_slave_new_compass_data(struct ak09916_instance_s *instance) {
    struct compass_register_s* st1_reg = get_compass_register(0x10);
    struct compass_register_s* st2_reg = get_compass_register(0x18);
    struct compass_register_s* cntl2_reg = get_compass_register(0x31);

    bool update_now = false;

    switch(cntl2_reg->value&0x1F) {
        case 0b00001:
            update_now = true;
            break;
        case 0b00010: // 10Hz
            update_now = compass_new_data_counter%10 == 0;
            break;
        case 0b00100: // 20Hz
            update_now = compass_new_data_counter%5 == 0;
            break;
        case 0b00110: // 50Hz
            update_now = compass_new_data_counter%2 == 0;
            break;
        case 0b01000: // 100Hz
            update_now = true;
            break;
    }

    if (compass_update_lock) {
        st1_reg->value |= (1<<1);
        update_now = false;
    }

    if (update_now) {
        float x,y,z;
        x = constrain_float(instance->meas.x*32752.0f/49120.0f, -32752.0f, 32752.0f);
        y = constrain_float(-instance->meas.y*32752.0f/49120.0f, -32752.0f, 32752.0f);
        z = constrain_float(-instance->meas.z*32752.0f/49120.0f, -32752.0f, 32752.0f);

        struct compass_register_s* meas_reg = get_compass_register(0x11);
        (meas_reg++)->value = (int16_t)x & 0xff;
        (meas_reg++)->value = ((int16_t)x >> 8) & 0xff;
        (meas_reg++)->value = (int16_t)y & 0xff;
        (meas_reg++)->value = ((int16_t)y >> 8) & 0xff;
        (meas_reg++)->value = (int16_t)z & 0xff;
        (meas_reg++)->value = ((int16_t)z >> 8) & 0xff;
        if (instance->meas.hofl) {
            st2_reg->value |= 1<<3; // HOFL
        }
        st1_reg->value |= 1; // DRDY

        if ((cntl2_reg->value&0x1F) == 1) {
            cntl2_reg->value = 0;
        }
    }

    compass_new_data_counter = (compass_new_data_counter+1) % 10;
}

bool ak09916_init(struct ak09916_instance_s *instance, struct icm20x48_instance_s* icm_instance)
{
    instance->icm_dev = icm_instance;

    icm20x48_i2c_slv_read_enable(icm_instance);
    if (!icm20x48_i2c_slv_set_passthrough(icm_instance, 0, AK09916_I2C_ADDR|0x80, 0x00, 2)) {
        return false;
    }
    usleep(10000);

    // Check the AK09916 WHO_I_AM registers
    bool failed = false;
    uint8_t byte;

    failed = failed || !icm20x48_i2c_slv_read(icm_instance, AK09916_I2C_ADDR, 0x00, &byte) || byte != 0x48;
    failed = failed || !icm20x48_i2c_slv_read(icm_instance, AK09916_I2C_ADDR, 0x01, &byte) || byte != 0x09;

    if (failed) {
        return false;
    }

    // Reset AK09916
    if (!icm20x48_i2c_slv_write(icm_instance, AK09916_I2C_ADDR, 0x32, 1)) {
        return false;
    }

    usleep(100000);

    // Place AK09916 in continuous measurement mode 4 (100hz), readback and verify
    if (!icm20x48_i2c_slv_write(icm_instance, AK09916_I2C_ADDR, 0x31, 0x08)) {
        return false;
    }

    failed = failed || !icm20x48_i2c_slv_read(icm_instance, AK09916_I2C_ADDR, 0x31, &byte) || byte != 0x08;

    if (failed) {
        return false;
    }

    //Configure the I2C master to read the status register and measuremnt data
    if (!icm20x48_i2c_slv_set_passthrough(icm_instance, 0, AK09916_I2C_ADDR|0x80, 0x10, 9)) {
        return false;
    }
    return true;
}

bool ak09916_update(struct ak09916_instance_s *instance)
{
    // TODO interrupt driven
    if ((icm20x48_read_reg(instance->icm_dev, ICM20948_REG_EXT_SLV_SENS_DATA_00) & (1<<0)) == 0) {
        return false;
    }

    uint8_t meas_bytes[6];
    for (uint8_t i=0; i<6; i++) {
        meas_bytes[i] = icm20x48_read_reg(instance->icm_dev, ICM20948_REG_EXT_SLV_SENS_DATA_01+i);
    }

    uint8_t st2 = icm20x48_read_reg(instance->icm_dev, ICM20948_REG_EXT_SLV_SENS_DATA_08);

    int16_t temp = 0;
    temp |= (uint16_t)meas_bytes[0];
    temp |= (uint16_t)meas_bytes[1]<<8;
    instance->meas.x = temp*42120.0f/32752.0f;

    temp = 0;
    temp |= (uint16_t)meas_bytes[2];
    temp |= (uint16_t)meas_bytes[3]<<8;
    instance->meas.y = temp*42120.0f/32752.0f;

    temp = 0;
    temp |= (uint16_t)meas_bytes[4];
    temp |= (uint16_t)meas_bytes[5]<<8;
    instance->meas.z = temp*42120.0f/32752.0f;

    instance->meas.hofl = (st2 & (1<<3)) != 0;
    i2c_slave_new_compass_data(instance);
    return true;
}


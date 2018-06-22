#include "driver_ms5525dso.h"
#include <common/bswap.h>

#ifdef MODULE_UAVCAN_DEBUG_ENABLED
#include <modules/uavcan_debug/uavcan_debug.h>
#define MS5525DSO_DEBUG(...) uavcan_send_debug_msg(UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_DEBUG, "MS5525DSO", __VA_ARGS__)
#else
#define MS5525DSO_DEBUG(...) {}
#endif

static const float Pmin = -1;
static const float Pmax = 1;
static const float Tres = 0.01;
static const float Pres = 0.0001;
static const uint32_t Q1 = 15;
static const uint32_t Q2 = 17;
static const uint32_t Q3 = 7;
static const uint32_t Q4 = 5;
static const uint32_t Q5 = 7;
static const uint32_t Q6 = 21;

static void ms5525dso_transact(struct ms5525dso_instance_s* instance, uint8_t txbyte, size_t rxlen, void* rxbuf);
uint32_t ms5525dso_sample(struct ms5525dso_instance_s* instance, bool d2);

bool ms5525dso_init(struct ms5525dso_instance_s* instance, uint8_t spi_idx, uint32_t select_line) {
    // Ensure sufficient power-up time has elapsed
    chThdSleep(MS2ST(100));

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 2000000, 8, SPI_DEVICE_FLAG_CPHA|SPI_DEVICE_FLAG_CPOL);

    // reset device
    ms5525dso_transact(instance, 0x1E, 0, NULL);

    chThdSleep(MS2ST(10));

    // read calibration data
    for (uint8_t i=0; i<8; i++) {
        ms5525dso_transact(instance, 0xA0+i*2, 2, &instance->C_coeff[i]);
        instance->C_coeff[i] = be16_to_cpu(instance->C_coeff[i]);
        MS5525DSO_DEBUG("C%u=%u", (uint32_t)i ,(uint32_t)instance->C_coeff[i]);
    }

    while(true) {
        const uint16_t C1 = instance->C_coeff[1];
        const uint16_t C2 = instance->C_coeff[2];
        const uint16_t C3 = instance->C_coeff[3];
        const uint16_t C4 = instance->C_coeff[4];
        const uint16_t C5 = instance->C_coeff[5];
        const uint16_t C6 = instance->C_coeff[6];

        uint32_t D1 = ms5525dso_sample(instance, false);
        uint32_t D2 = ms5525dso_sample(instance, true);
        int64_t dT = D2-(int64_t)C5*(1<<Q5);
        int32_t temp = 2000+(dT*C6)/(1<<Q6);

        int64_t off = (int64_t)C2*(1<<Q2)/*+(C4*dT)/(1<<Q4)*/;
        int64_t sens = (int64_t)C1*(1<<Q1)/*+(C3*dT)/(1<<Q3)*/;
        int32_t P = ((D1*sens)/(1<<21)-off)/(1<<15);
        uavcan_send_debug_keyvalue("aspd", P*Pres);
        MS5525DSO_DEBUG("T=%d", temp);

        chThdSleep(MS2ST(50));
    }

    return true;
}

uint32_t ms5525dso_sample(struct ms5525dso_instance_s* instance, bool d2) {
    // conversion
    spi_device_begin(&instance->spi_dev);
    uint8_t txbyte = d2?0x58:0x48;
    spi_device_send(&instance->spi_dev, 1, &txbyte);
    chThdSleep(MS2ST(10));
    spi_device_end(&instance->spi_dev);

    // read
    union {
        uint8_t bytes[4];
        uint32_t value;
    } result;
    result.bytes[0] = 0;
    ms5525dso_transact(instance, 0, 3, &result.bytes[1]);
    return be32_to_cpu(result.value);
}


static void ms5525dso_transact(struct ms5525dso_instance_s* instance, uint8_t txbyte, size_t rxlen, void* rxbuf) {
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, 1, &txbyte);
    spi_device_receive(&instance->spi_dev, rxlen, rxbuf);
    spi_device_end(&instance->spi_dev);
}

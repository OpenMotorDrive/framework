#include "driver_ms5611.h"
#include <common/helpers.h>
#include <modules/uavcan_debug/uavcan_debug.h>

static bool ms5611_read_prom(struct ms5611_instance_s* instance);
static uint32_t ms5611_read_adc(struct ms5611_instance_s* instance);
static void ms5611_cmd(struct ms5611_instance_s* instance, uint8_t cmd);
static void ms5611_read(struct ms5611_instance_s* instance, uint8_t addr, uint8_t n, uint8_t* buf);
static bool crc4(uint16_t *prom);

bool ms5611_init(struct ms5611_instance_s* instance, uint8_t spi_idx, uint32_t select_line)
{
    // Ensure sufficient power-up time has elapsed
    chThdSleep(MS2ST(100));

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 20000000, 8, SPI_DEVICE_FLAG_CPHA|SPI_DEVICE_FLAG_CPOL);

    // Reset device
    ms5611_cmd(instance, MS5611_CMD_RESET);

    chThdSleep(MS2ST(20));

    if (!ms5611_read_prom(instance)) {
        return false;
    }
    return true;
}

bool ms5611_measure_temperature(struct ms5611_instance_s* instance) {
    if (instance->temperature_read_started || instance->pressure_read_started) {
        return false;
    }
    ms5611_cmd(instance, MS5611_CMD_CVT_D2_1024);
    instance->temperature_read_started = true;
    return true;
}

bool ms5611_measure_pressure(struct ms5611_instance_s* instance) {
    if (instance->temperature_read_started || instance->pressure_read_started) {
        return false;
    }
    ms5611_cmd(instance, MS5611_CMD_CVT_D1_1024);
    instance->pressure_read_started = true;
    return true;
}

void ms5611_accum_temperature(struct ms5611_instance_s* instance)
{
    if (!instance->temperature_read_started) {
        return;
    }
    instance->sD2 += ms5611_read_adc(instance);
    instance->sD2_count++;

    instance->temperature_read_started = false;
}

void ms5611_accum_pressure(struct ms5611_instance_s* instance)
{
    if (!instance->pressure_read_started) {
        return;
    }
    instance->sD1 += ms5611_read_adc(instance);
    instance->sD1_count++;

    instance->pressure_read_started = false;
}

int32_t ms5611_read_temperature(struct ms5611_instance_s* instance)
{
    if (!instance->sD2_count) {
        return 0;
    }
    uint32_t D2 = (uint32_t)(instance->sD2 / instance->sD2_count);
    instance->sD2 = 0;
    instance->sD2_count = 0;

    /* temperature offset (in ADC units) */
    int32_t dT = (int32_t)D2 - ((int32_t)instance->prom.s.c5_reference_temp << 8);

    /* absolute temperature in centidegrees - note intermediate value is outside 32-bit range */
    instance->TEMP = 2000 + (int32_t)(((int64_t)dT * instance->prom.s.c6_temp_coeff_temp) >> 23);

    /* Perform MS5611 Caculation */

    instance->OFF  = ((int64_t)instance->prom.s.c2_pressure_offset << 16) + (((int64_t)instance->prom.s.c4_temp_coeff_pres_offset * dT) >> 7);
    instance->SENS = ((int64_t)instance->prom.s.c1_pressure_sens << 15) + (((int64_t)instance->prom.s.c3_temp_coeff_pres_sens * dT) >> 8);

    /* MS5611 temperature compensation */

    if (instance->TEMP < 2000) {

        int32_t T2 = SQ(dT) >> 31;

        int64_t f = SQ((int64_t)instance->TEMP - 2000);
        int64_t OFF2 = 5 * f >> 1;
        int64_t SENS2 = 5 * f >> 2;

        if (instance->TEMP < -1500) {

            int64_t f2 = SQ(instance->TEMP + 1500);
            OFF2 += 7 * f2;
            SENS2 += 11 * f2 >> 1;
        }

        instance->TEMP -= T2;
        instance->OFF  -= OFF2;
        instance->SENS -= SENS2;
    }

    return instance->TEMP;
}

int32_t ms5611_read_pressure(struct ms5611_instance_s* instance) {
    if (!instance->sD1_count) {
        return 0;
    }
    int64_t P = instance->sD1 / instance->sD1_count;
    instance->sD1_count = 0;
    instance->sD1 = 0;
	P = (((P * instance->SENS) >> 21) - instance->OFF) >> 15;

    instance->pressure_read_started = false;
    return P;
}

static bool ms5611_read_prom(struct ms5611_instance_s* instance) {
    uint8_t val[3] = {0};
    uint8_t addr = MS5611_CMD_PROM_READ;
    bool  prom_read_failed = true;
    for (uint8_t i = 0; i < 8; i++) {
        ms5611_read(instance, addr, 2, val);
        addr += 2;
        instance->prom.c[i] = (val[0] << 8) | val[1];
        if (instance->prom.c[i] != 0) {
            prom_read_failed = false;
        }
    }
    if (prom_read_failed) {
        return false;
    }
    if (crc4(instance->prom.c)) {
        return true;
    }
    return false;
}

static bool crc4(uint16_t *prom)
{
	int16_t cnt;
	uint16_t n_rem;
	uint16_t crc_read;
	uint8_t n_bit;

	n_rem = 0x00;

	/* save the read crc */
	crc_read = prom[7];

	/* remove CRC byte */
	prom[7] = (0xFF00 & (prom[7]));

	for (cnt = 0; cnt < 16; cnt++) {
		/* uneven bytes */
		if (cnt & 1) {
			n_rem ^= (uint8_t)((prom[cnt >> 1]) & 0x00FF);

		} else {
			n_rem ^= (uint8_t)(prom[cnt >> 1] >> 8);
		}

		for (n_bit = 8; n_bit > 0; n_bit--) {
			if (n_rem & 0x8000) {
				n_rem = (n_rem << 1) ^ 0x3000;

			} else {
				n_rem = (n_rem << 1);
			}
		}
	}

	/* final 4 bit remainder is CRC value */
	n_rem = (0x000F & (n_rem >> 12));
	prom[7] = crc_read;

	/* return true if CRCs match */
	return (0x000F & crc_read) == (n_rem ^ 0x00);
}

static uint32_t ms5611_read_adc(struct ms5611_instance_s* instance) {
    uint8_t val[3] = {0};
    ms5611_read(instance, MS5611_CMD_ADC_READ, 3, val);
    return (val[0] << 16) | (val[1] << 8) | val[2];
}

static void ms5611_cmd(struct ms5611_instance_s* instance, uint8_t cmd) {
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(cmd), &cmd);
    spi_device_end(&instance->spi_dev);
}

static void ms5611_read(struct ms5611_instance_s* instance, uint8_t addr, uint8_t n, uint8_t* buf)
{
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, 1, &addr);
    spi_device_receive(&instance->spi_dev, n, buf);
    spi_device_end(&instance->spi_dev);
}



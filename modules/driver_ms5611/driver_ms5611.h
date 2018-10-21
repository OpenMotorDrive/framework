#pragma once

#include <modules/spi_device/spi_device.h>

#define MS5611_CMD_RESET                    0x1E
#define MS5611_CMD_CVT_D1_256               0x40
#define MS5611_CMD_CVT_D1_512               0x42
#define MS5611_CMD_CVT_D1_1024              0x44
#define MS5611_CMD_CVT_D1_2048              0x46
#define MS5611_CMD_CVT_D1_4096              0x48
#define MS5611_CMD_CVT_D2_256               0x50
#define MS5611_CMD_CVT_D2_512               0x52
#define MS5611_CMD_CVT_D2_1024              0x54
#define MS5611_CMD_CVT_D2_2048              0x56
#define MS5611_CMD_CVT_D2_4096              0x58
#define MS5611_CMD_ADC_READ                 0x00
#define MS5611_CMD_PROM_READ                0xA0
/**
 * Calibration PROM as reported by the device.
 */
#pragma pack(push,1)
struct prom_s {
	uint16_t factory_setup;
	uint16_t c1_pressure_sens;
	uint16_t c2_pressure_offset;
	uint16_t c3_temp_coeff_pres_sens;
	uint16_t c4_temp_coeff_pres_offset;
	uint16_t c5_reference_temp;
	uint16_t c6_temp_coeff_temp;
	uint16_t serial_and_crc;
};

/**
 * Grody hack for crc4()
 */
union prom_u {
	uint16_t c[8];
	struct prom_s s;
};
#pragma pack(pop)

struct ms5611_instance_s {
    struct spi_device_s spi_dev;
    union prom_u prom;
    int32_t TEMP;
	int64_t	OFF;
	int64_t	SENS;
	int64_t sD1;
	uint64_t sD2;
	uint8_t sD1_count;
	uint8_t sD2_count;
    bool prom_read_ok;
    bool temperature_read_started;
    bool pressure_read_started;
};


bool ms5611_init(struct ms5611_instance_s* instance, uint8_t spi_idx, uint32_t select_line);
bool ms5611_measure_temperature(struct ms5611_instance_s* instance);
bool ms5611_measure_pressure(struct ms5611_instance_s* instance);
void ms5611_accum_temperature(struct ms5611_instance_s* instance);
void ms5611_accum_pressure(struct ms5611_instance_s* instance);
int32_t ms5611_get_temperature(struct ms5611_instance_s* instance);
int32_t ms5611_get_pressure(struct ms5611_instance_s* instance);

#pragma once

#include <hal.h>
#include <stdbool.h>
#include <stdint.h>

#define OMD_SPI_FLAG_CPHA     (1<<0)
#define OMD_SPI_FLAG_CPOL     (1<<1)
#define OMD_SPI_FLAG_LSBFIRST (1<<2)
#define OMD_SPI_FLAG_SELPOL   (1<<3)

struct spi_device_s {
    uint32_t max_speed_hz;
    uint32_t sel_line;
    uint8_t bus_idx;
    uint8_t data_size;
    uint8_t flags;
    bool bus_acquired;

    SPIConfig spiconf;
};


bool spi_device_init(struct spi_device_s* dev, uint8_t bus_idx, uint32_t sel_line, uint32_t max_speed_hz, uint8_t data_size, uint8_t flags);
void spi_device_begin(struct spi_device_s* dev);
void spi_device_send(struct spi_device_s* dev, uint32_t n, const void* txbuf);
void spi_device_receive(struct spi_device_s* dev, uint32_t n, void* txbuf);
void spi_device_exchange(struct spi_device_s* dev, uint32_t n, const void* txbuf, void* rxbuf);
void spi_device_end(struct spi_device_s* dev);

#pragma once

#include <ch.h>
#include <spi_device/spi_device.h>

enum dw1000_prf_t {
    DW1000_PRF_16MHZ,
    DW1000_PRF_64MHZ
};

enum dw1000_preamble_t {
    DW1000_PREAMBLE_64,
    DW1000_PREAMBLE_128,
    DW1000_PREAMBLE_256,
    DW1000_PREAMBLE_512,
    DW1000_PREAMBLE_1024,
    DW1000_PREAMBLE_1536,
    DW1000_PREAMBLE_2048,
    DW1000_PREAMBLE_4096
};

enum dw1000_channel_t {
    DW1000_CHANNEL_1 = 1,
    DW1000_CHANNEL_2 = 2,
    DW1000_CHANNEL_3 = 3,
    DW1000_CHANNEL_4 = 4,
    DW1000_CHANNEL_5 = 5,
    DW1000_CHANNEL_7 = 7
};

enum dw1000_data_rate_t {
    DW1000_DATA_RATE_110K,
    DW1000_DATA_RATE_850K,
    DW1000_DATA_RATE_6_8M
};

struct dw1000_config_s {
    enum dw1000_prf_t prf;
    enum dw1000_preamble_t preamble;
    enum dw1000_channel_t channel;
    enum dw1000_data_rate_t data_rate;
    uint8_t pcode;
};

enum dw1000_rx_frame_err_code_s {
    DW1000_RX_ERROR_NONE=0,
    DW1000_RX_ERROR_NULLPTR,
    DW1000_RX_ERROR_NO_FRAME_PRESENT,
    DW1000_RX_ERROR_PROVIDED_BUFFER_TOO_SMALL,
    DW1000_RX_ERROR_RXOVRR

};

struct dw1000_rx_frame_info_s {
    enum dw1000_rx_frame_err_code_s err_code;
    uint16_t len;
    uint64_t timestamp;
    int32_t rx_ttcko;
    uint32_t rx_ttcki;
};

struct dw1000_instance_s {
    struct spi_device_s spi_dev;
    uint32_t reset_line;
    struct dw1000_config_s config;
};

void dw1000_init(struct dw1000_instance_s* instance, uint8_t spi_idx, uint32_t select_line, uint32_t reset_line);
struct dw1000_rx_frame_info_s dw1000_receive(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf);
void dw1000_transmit(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf, bool expect_response);
bool dw1000_scheduled_transmit(struct dw1000_instance_s* instance, uint64_t transmit_time, uint32_t buf_len, void* buf, bool expect_response);
void dw1000_try_receive(struct dw1000_instance_s* instance);
void dw1000_rx_enable(struct dw1000_instance_s* instance);
void dw1000_disable_transceiver(struct dw1000_instance_s* instance);
void dw1000_handle_interrupt(struct dw1000_instance_s* instance);

uint64_t dw1000_get_tx_stamp(struct dw1000_instance_s* instance);

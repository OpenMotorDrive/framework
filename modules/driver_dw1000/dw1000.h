#pragma once

#include <ch.h>
#include <modules/spi_device/spi_device.h>

#define DW1000_TIME_TO_METERS 0.0046917639786159f
#define DW1000_METERS_TO_TIME 213.13945f
#define DW1000_TIMESTAMP_MAX 0xffffffffff
#define DW1000_IRQ_MASK(x) (1UL<<x)

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

enum dw1000_sys_status_t {
    DW1000_SYS_STATUS_CPLOCK = 1,
    DW1000_SYS_STATUS_ESYNCR,
    DW1000_SYS_STATUS_AAT,
    DW1000_SYS_STATUS_TXFRB,
    DW1000_SYS_STATUS_TXPRS,
    DW1000_SYS_STATUS_TXPHS,
    DW1000_SYS_STATUS_TXFRS,
    DW1000_SYS_STATUS_RXPRD,
    DW1000_SYS_STATUS_RXSFDD,
    DW1000_SYS_STATUS_LDEDONE,
    DW1000_SYS_STATUS_RXPHD,
    DW1000_SYS_STATUS_RXPHE,
    DW1000_SYS_STATUS_RXDFR,
    DW1000_SYS_STATUS_RXFCG,
    DW1000_SYS_STATUS_RXFCE,
    DW1000_SYS_STATUS_RXRFSL,
    DW1000_SYS_STATUS_RXRFTO,
    DW1000_SYS_STATUS_LDEERR, //18
    DW1000_SYS_STATUS_RXOVRR = 20, //20
    DW1000_SYS_STATUS_RXPTO,
    DW1000_SYS_STATUS_GPIOIRQ,
    DW1000_SYS_STATUS_SLP2INIT,
    DW1000_SYS_STATUS_RFPLLLL,
    DW1000_SYS_STATUS_PLLHILO,
    DW1000_SYS_STATUS_RXSFDTO,
    DW1000_SYS_STATUS_HPDWARN,
    DW1000_SYS_STATUS_TXBERR,
    DW1000_SYS_STATUS_AFFREJ
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
    uint16_t ant_delay;
    bool std_data_length;
    uint8_t tx_power;
};

enum dw1000_rx_frame_err_code_s {
    DW1000_RX_ERROR_NONE=0,
    DW1000_RX_ERROR_NULLPTR,
    DW1000_RX_ERROR_NO_FRAME_PRESENT,
    DW1000_RX_ERROR_PROVIDED_BUFFER_TOO_SMALL,
    DW1000_RX_ERROR_RXOVRR,
    DW1000_RX_ERROR_TIMESTAMP_PENDING
};

struct dw1000_rx_frame_info_s {
    enum dw1000_rx_frame_err_code_s err_code;
    uint16_t len;
    int64_t timestamp;
    int32_t rx_ttcko;
    uint32_t rx_ttcki;
    uint16_t std_noise;
    uint16_t fp_ampl1;
    uint16_t fp_ampl2;
    uint16_t fp_ampl3;
    uint16_t cir_pwr;
    uint16_t rxpacc_corrected;
    float rssi_est;
    float fp_rssi_est;
};

struct dw1000_instance_s {
    struct spi_device_s spi_dev;
    uint32_t reset_line;
    uint8_t t_meas_23c;
    struct {
        uint8_t v_meas_3v3;
        uint8_t v_meas_3v7;
    };
    struct dw1000_config_s config;
};

int64_t dw1000_wrap_timestamp(int64_t ts);
void dw1000_init(struct dw1000_instance_s* instance, uint8_t spi_idx, uint32_t select_line, uint32_t reset_line);
struct dw1000_rx_frame_info_s dw1000_receive(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf);
struct dw1000_rx_frame_info_s dw1000_receive_data_only(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf);
void dw1000_transmit(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf, bool expect_response);
bool dw1000_scheduled_transmit(struct dw1000_instance_s* instance, uint64_t transmit_time, uint32_t buf_len, void* buf, bool expect_response);
void dw1000_try_receive(struct dw1000_instance_s* instance);
void dw1000_rx_enable(struct dw1000_instance_s* instance);
void dw1000_rx_softreset(struct dw1000_instance_s* instance);
void dw1000_disable_transceiver(struct dw1000_instance_s* instance);
void dw1000_handle_interrupt(struct dw1000_instance_s* instance);
void dw1000_clear_status(struct dw1000_instance_s* instance, uint32_t status_mask);
uint32_t dw1000_get_status(struct dw1000_instance_s* instance);
void dw1000_swap_rx_buffers(struct dw1000_instance_s* instance);
systime_t dw1000_timestamp_to_systime(struct dw1000_instance_s* instance, uint64_t dw1000_timestamp);
systime_t dw1000_ticks_to_systicks(uint64_t dw1000_ticks);
uint64_t systicks_to_dw1000_ticks(systime_t systicks);

uint64_t dw1000_get_tx_stamp(struct dw1000_instance_s* instance);
int64_t dw1000_get_sys_time(struct dw1000_instance_s* instance);
uint16_t dw1000_get_ant_delay(struct dw1000_instance_s* instance);
void dw1000_set_ant_delay(struct dw1000_instance_s* instance, uint16_t ant_delay);
float dw1000_get_temp(struct dw1000_instance_s* instance);
void dw1000_set_tx_power(struct dw1000_instance_s* instance, uint8_t tx_power);
float dw1000_get_rssi_est(struct dw1000_instance_s* instance, uint16_t cir_pwr, uint16_t rxpacc);
float dw1000_get_fp_rssi_est(struct dw1000_instance_s* instance, uint16_t fp_ampl1, uint16_t fp_ampl2, uint16_t fp_ampl3, uint16_t rxpacc);
int64_t dw1000_correct_tstamp(struct dw1000_instance_s* instance, float estRxPwr, int64_t ts);
void dw1000_setup_irq(struct dw1000_instance_s* instance, uint32_t status_mask);
uint32_t dw1000_get_irq_mask(struct dw1000_instance_s* instance);
uint32_t dw1000_get_sys_config(struct dw1000_instance_s* instance);

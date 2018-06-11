#include "dw1000.h"
#include "dw1000_internal.h"
#include <hal.h>
#include <modules/timing/timing.h>
#include <modules/uavcan/uavcan.h>
#include <common/bswap.h>
#include <common/helpers.h>

#ifndef DW1000_LEDS_ENABLED
#define DW1000_LEDS_ENABLED FALSE
#endif

#include <stdio.h>

static void dw1000_config(struct dw1000_instance_s* instance);
static void dw1000_write(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, const void* buf);
static void dw1000_read(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, void* buf);
static void dw1000_hard_reset(struct dw1000_instance_s* instance);
static void dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(struct dw1000_instance_s* instance, bool transceiver_disable);
static void dw1000_clear_double_buffered_status_bits(struct dw1000_instance_s* instance);
static void dw1000_write8(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint8_t val);
static void dw1000_write16(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint16_t val);
static void dw1000_write32(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t val);
static void dw1000_otp_read(struct dw1000_instance_s* instance, uint16_t otp_addr, uint8_t otp_field_len, void* buf);
static void dw1000_load_ldotune(struct dw1000_instance_s* instance);
static void dw1000_load_lde_microcode(struct dw1000_instance_s* instance);
static void dw1000_clock_force_sys_xti(struct dw1000_instance_s* instance);
static void dw1000_clock_enable_all_seq(struct dw1000_instance_s* instance);

void dw1000_handle_interrupt(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    // Read SYS_STATUS
    struct dw1000_sys_status_s sys_status;
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Clear SYS_STATUS bits
    dw1000_write(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);
}

//call this method after every subtraction of timestamps
int64_t dw1000_wrap_timestamp(int64_t ts) {
    return ts & (((uint64_t)1<<40)-1);
}

void dw1000_init(struct dw1000_instance_s* instance, uint8_t spi_idx, uint32_t select_line, uint32_t reset_line) {
    if (!instance) {
        return;
    }

    // NOTE: Per DW1000 User Manual Section 2.3.2:
    // SPI accesses from an external microcontroller are possible in the INIT state, but these
    // are limited to a SPICLK input frequency of no greater than 3 MHz. Care should be taken
    // not to have an active SPI access in progress at the CLKPLL lock time (i.e. at t = 5 µs) when
    // the automatic switch from the INIT state to the IDLE state is occurring, because the
    // switch-over of clock source can cause bit errors in the SPI transactions.

    // This is handled by sleeping inside of dw1000_hard_reset

    spi_device_init(&instance->spi_dev, spi_idx, select_line, 3000000, 8, 0);
    instance->reset_line = reset_line;
    instance->config.prf = DW1000_PRF_64MHZ;
    instance->config.preamble = DW1000_PREAMBLE_128;
    instance->config.channel = DW1000_CHANNEL_7;
    instance->config.data_rate = DW1000_DATA_RATE_6_8M;
    instance->config.pcode = dw1000_conf_get_default_pcode(instance->config);
    instance->config.std_data_length = false;

    dw1000_hard_reset(instance);

    // NOTE: Per DW1000 API: this makes OTP read reliable
    dw1000_clock_force_sys_xti(instance);

    // DW1000 User Manual Section 2.5.5.10
    dw1000_load_lde_microcode(instance);

    // DW1000 User Manual Section 2.5.5.11
    dw1000_load_ldotune(instance);

    // Load temperature and voltage sensor calibrations
    {
        uint8_t buf[2];
        dw1000_otp_read(instance, 0x008, 2, buf);
        instance->v_meas_3v3 = buf[0];
        instance->v_meas_3v7 = buf[1];
        dw1000_otp_read(instance, 0x009, 1, buf);
        instance->t_meas_23c = buf[0];
    }

    // Switch back to normal clock
    dw1000_clock_enable_all_seq(instance);

    dw1000_config(instance);
}


uint32_t dw1000_get_sys_config(struct dw1000_instance_s* instance) {
    uint32_t sys_cfg;
    dw1000_read(instance, DW1000_SYSTEM_CONFIGURATION_FILE, 0, sizeof(sys_cfg), &sys_cfg);
    return sys_cfg;
}

static void dw1000_config(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    struct dw1000_config_s config = instance->config;

    // Register-by-register:
    //   ID     Len  Name        Comment
    // 0x00       4  DEV_ID      not config
    // 0x01       8  EUI         TODO
    // 0x03       4  PANADR      TODO
    // 0x04       4  SYS_CFG     -
    {
        // [0x00:0x03] SYS_CFG
        // Enable rx auto-re-enable and rx double-buffering
        struct dw1000_sys_cfg_s sys_cfg;
        memset(&sys_cfg, 0, sizeof(sys_cfg));
        sys_cfg.HIRQ_POL = 1;
        sys_cfg.RXAUTR = 1;
        sys_cfg.DIS_STXP = 1;
        if (!instance->config.std_data_length) {
            sys_cfg.PHR_MODE = 0x3; //setup to non standard data length to vary between 0-1023
        }
        dw1000_write(instance, DW1000_SYSTEM_CONFIGURATION_FILE, 0, sizeof(sys_cfg), &sys_cfg);
    }
    // 0x06       5  SYS_TIME    not config
    // 0x08       5  TX_FCTRL    -
    {
        // [0x00:0x04] TX_FCTRL
        struct dw1000_tx_fctrl_s tx_fctrl = dw1000_conf_tx_fctrl(config);
        dw1000_write(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);
    }
    // 0x09    1024  TX_BUFFER   not config
    // 0x0A       5  DX_TIME     not config
    // 0x0C       2  RX_FWTO     -
    {
        // [0x00:0x01] RX_FWTO
        dw1000_write16(instance, 0x0C, 0, 0);
    }
    // 0x0D       4  SYS_CTRL    -
    {
        // [0x00:0x03] SYS_CTRL
        dw1000_write32(instance, 0x0D, 0, 0);
    }
    // 0x0E       4  SYS_MASK    -
    {
        // [0x00:0x03] SYS_MASK
        dw1000_write32(instance, 0x0E, 0, 0);
    }
    // 0x0F       5  SYS_STATUS  not config
    // 0x10       4  RX_FINFO    not config
    // 0x11    1024  RX_BUFFER   not config
    // 0x12       8  RX_FQUAL    not config
    // 0x13       4  RX_TTCKI    not config
    // 0x14       5  RX_TTCKO    not config
    // 0x15      14  RX_TIME     not config
    // 0x17      10  TX_TIME     not config
    // 0x18       2  TX_ANTD
    {
        // [0x00:0x01] TX_ANTD
        dw1000_write16(instance, 0x18, 0, config.ant_delay);
    }
    // 0x19       5  SYS_STATE   not config
    // 0x1A       4  ACK_RESP_T  -
    {
        // [0x00:0x03] ACK_RESP_T
        dw1000_write32(instance, 0x1A, 0, 0);
    }
    // 0x1D       4  RX_SNIFF    -
    {
        // [0x00:0x03] RX_SNIFF
        dw1000_write32(instance, 0x1D, 0, 0);
    }
    // 0x1F       4  CHAN_CTRL   -
    {
        // [0x00:0x03] CHAN_CTRL
        struct dw1000_chan_ctrl_s chan_ctrl = dw1000_conf_chan_ctrl(config);
        dw1000_write(instance, 0x1F, 0x00, sizeof(chan_ctrl), &chan_ctrl);
    }
    // 0x21      41  USR_SFD     -
    {
        // [0x00] SFD_LENGTH
        dw1000_write8(instance, 0x21, 0x00, dw1000_conf_sfd_length(config));
        // [0x01:0x28] ignored due to TNSSFD/RNSSFD = 0 in CHAN_CTRL
    }
    // 0x23      32  AGC_CTRL    -
    {
        // [0x00:0x01] reserved
        // [0x02:0x03] AGC_CTRL1
        dw1000_write16(instance, 0x23, 0x02, 0);
        // [0x04:0x05] AGC_TUNE1
        dw1000_write16(instance, 0x23, 0x04, dw1000_conf_agc_tune1(config));
        // [0x06:0x0B] reserved
        // [0x0C:0x0F] AGC_TUNE2
        dw1000_write32(instance, 0x23, 0x0C, 0x2502A907);
        // [0x10:0x1D] reserved
        // [0x1E:0x1F] AGC_TUNE3
        dw1000_write16(instance, 0x23, 0x1E, 0x0035);
    }
    // 0x24      12  EXT_SYNC    -
    {
        // [0x00:0x03] EC_CTRL
        dw1000_write32(instance, 0x24, 0x00, 0);
        // [0x04:0x07] EC_RXTC read-only
        // [0x08:0x0B] EC_GOLP read-only
    }
    // 0x25    4064  ACC_MEM     not config
    // 0x26      44  GPIO_CTRL   TODO
    {
#if DW1000_LEDS_ENABLED
        dw1000_write16(instance, 0x26, 0x00, (uint16_t)0b01010101<<6);
        dw1000_write8(instance, 0x26, 0x08, 0xf0);
#endif
    }
    // 0x27      44  DRX_CONF    -
    {
        // [0x00:0x01] reserved
        // [0x02:0x03] DRX_TUNE0b
        dw1000_write16(instance, 0x27, 0x02, dw1000_conf_drx_tune0b(config));
        // [0x04:0x05] DRX_TUNE1a
        dw1000_write16(instance, 0x27, 0x04, dw1000_conf_drx_tune1a(config));
        // [0x06:0x07] DRX_TUNE1b
        dw1000_write16(instance, 0x27, 0x06, dw1000_conf_drx_tune1b(config));
        // [0x08:0x0B] DRX_TUNE2
        dw1000_write32(instance, 0x27, 0x08, dw1000_conf_drx_tune2(config));
        // [0x0C:0x1F] reserved
        // [0x20:0x21] DRX_SFDTOC
        dw1000_write16(instance, 0x27, 0x20, dw1000_conf_drx_sfdtoc(config));
        // [0x22:0x23] reserved
        // [0x24:0x25] DRX_PRETOC
        dw1000_write16(instance, 0x27, 0x24, 0);
        // [0x26:0x27] DRX_TUNE4H
        dw1000_write16(instance, 0x27, 0x26, dw1000_conf_drx_tune4h(config));
    }
    // 0x28      58  RF_CONF     -
    {
        // [0x00:0x03] RF_CONF
        dw1000_write32(instance, 0x28, 0x00, 0);
        // [0x04:0x0A] reserved
        // [0x0B] RF_RXCTRLH
        dw1000_write8(instance, 0x28, 0x0B, dw1000_conf_rf_rxctrlh(config));
        // [0x0C:0x0F] RF_TXCTRL
        dw1000_write32(instance, 0x28, 0x0C, dw1000_conf_rf_txctrl(config));
        // [0x10:0x1F] reserved
        // [0x2C:0x2F] RF_STATUS
        // [0x30:0x34] LDOTUNE NOTE: written by dw1000_load_ldotune
    }
    // 0x2A      52  TX_CAL      -
    {
        // [0x00:0x01] TC_SARC
        // [0x03:0x06] TC_SARL
        // [0x06:0x07] TC_SARW
        // [0x08:0x0A] undocumented
        // [0x0B] TC_PGDELAY
        dw1000_write8(instance, 0x2A, 0x0B, dw1000_conf_tc_pgdelay(config));
        // [0x0C] TC_PGTEST
        dw1000_write8(instance, 0x2A, 0x0C, 0);
    }
    // 0x2B      21  FS_CTRL     -
    {
        // [0x00:0x06] reserved
        // [0x07:0x0A] FS_PLLCFG
        dw1000_write32(instance, 0x2B, 0x07, dw1000_conf_fs_pllcfg(config));
        // [0x0B] FS_PLLTUNE
        dw1000_write8(instance, 0x2B, 0x0B, dw1000_conf_fs_plltune(config));
        // [0x0C:0x0D] reserved
        // [0x0E] FS_XTALT
        dw1000_write8(instance, 0x2B, 0x0E, 0x6F);
    }
    // 0x2C      12  AON         not config
    // 0x2D      18  OTP_IF      not config
    // 0x2E       -  LDE_IF      -
    {
        // [0x0000:0x0001] LDE_THRESH
        // [0x0806] LDE_CFG1
        dw1000_write8(instance, 0x2E, 0x0806, 0x0D);
        // [0x1000:0x1001] LDE_PPINDX
        // [0x1002:0x1003] LDE_PPAMPL
        // [0x1804:0x1805] LDE_RXANTD
        dw1000_write16(instance, 0x2E, 0x1804, config.ant_delay);
        // [0x1806:0x1807] LDE_CFG2
        dw1000_write16(instance, 0x2E, 0x1806, dw1000_conf_lde_cfg2(config));
        // [0x2804:0x2805] LDE_REPC
        dw1000_write16(instance, 0x2E, 0x2804, dw1000_conf_lde_repc(config));
    }
    // 0x2F      41  DIG_DIAG    not config
    // 0x36      48  PMSC        -
    {
        // [0x00:0x03] PMSC_CTRL0
#if DW1000_LEDS_ENABLED
        dw1000_write8(instance, 0x36, 0x02, 0b10111111);
#endif
        // [0x04:0x07] PMSC_CTRL1
        // [0x08:0x0B] reserved
        // [0x0C] PMSC_SNOZT
        // [0x10:0x25] reserved
        // [0x26:0x27] PMSC_TXFSEQ
        // [0x28:0x2B] PMSC_LEDC
#if DW1000_LEDS_ENABLED
        dw1000_write8(instance, 0x36, 0x28, 1);
        dw1000_write8(instance, 0x36, 0x29, 1);
#endif
    }
}

void dw1000_setup_irq(struct dw1000_instance_s* instance, uint32_t status_mask)
{
    if (!instance) {
        return;
    }

    dw1000_write32(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, status_mask);
    dw1000_clear_status(instance, status_mask);
}

uint32_t dw1000_get_irq_mask(struct dw1000_instance_s* instance)
{
    uint32_t status_mask = 0;
    dw1000_read(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(status_mask), &status_mask);
    return status_mask;
}

void dw1000_clear_status(struct dw1000_instance_s* instance, uint32_t status_mask)
{
    dw1000_write(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(status_mask), &status_mask);
}

uint32_t dw1000_get_status(struct dw1000_instance_s* instance)
{
    uint32_t sys_status = 0;
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);
    return sys_status;
}

void dw1000_rx_enable(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    struct dw1000_sys_status_s sys_status;

    // Read SYS_STATUS to check that HSRBP == ICRBP
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    if (sys_status.HSRBP != sys_status.ICRBP) {
        // Issue the HRBPT command
        dw1000_swap_rx_buffers(instance);
    }

    // Set RXENAB bit
    struct dw1000_sys_ctrl_s sys_ctrl;
    memset(&sys_ctrl, 0, sizeof(sys_ctrl));
    sys_ctrl.RXENAB = 1;
    dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
}

void dw1000_rx_softreset(struct dw1000_instance_s* instance) {
    uint32_t pmsc_ctrl0;
    dw1000_read(instance, DW1000_POWER_MANAGEMENT_AND_SYSTEM_CONTROL_FILE, 0, 4, &pmsc_ctrl0);
    //Clear SOFTRESET for RX
    pmsc_ctrl0 = (pmsc_ctrl0 & ~(1U<<28));
    dw1000_write(instance, DW1000_POWER_MANAGEMENT_AND_SYSTEM_CONTROL_FILE, 0, 4, &pmsc_ctrl0);
    //reenable receiver
    pmsc_ctrl0 = (pmsc_ctrl0 | (1U<<28));
    dw1000_write(instance, DW1000_POWER_MANAGEMENT_AND_SYSTEM_CONTROL_FILE, 0, 4, &pmsc_ctrl0);
}


struct dw1000_rx_frame_info_s dw1000_receive(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf) {
    struct dw1000_rx_frame_info_s ret;
    memset(&ret,0,sizeof(ret));
    if (!instance || !buf) {
        ret.err_code = DW1000_RX_ERROR_NULLPTR;
        return ret;
    }

    // DW1000 User Manual Figure 14

    struct dw1000_sys_status_s sys_status;
    struct dw1000_rx_finfo_s rx_finfo;
    struct dw1000_rx_fqual_s rx_fqual;
    uint16_t fp_ampl1;
    uint16_t fp_ampl2;
    uint16_t fp_ampl3;
    uint16_t rxpacc_nosat;
    uint16_t rxpacc_corrected;
    uint16_t rxpacc;
    uint16_t cir_pwr;

    // Read SYS_STATUS
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Check RXOVRR
    if (sys_status.RXOVRR || sys_status.LDEERR || sys_status.RXFCE || sys_status.RXPTO || sys_status.RXSFDTO || sys_status.RXPHE || sys_status.RXRFSL || sys_status.RXRFTO) {
        // Frames must be discarded (do not read frames) due to corrupted registers and TRXOFF command issued
        dw1000_disable_transceiver(instance);
        dw1000_swap_rx_buffers(instance);
        dw1000_rx_softreset(instance);
    
        // Receiver must be reset to exit errored state
        dw1000_rx_enable(instance);

        memset(&ret,0,sizeof(ret));
        ret.err_code = DW1000_RX_ERROR_RXOVRR;
        return ret;
    }

    // Check if a good frame is in the buffer
    if (!sys_status.RXFCG) {
        ret.err_code = DW1000_RX_ERROR_NO_FRAME_PRESENT;
        return ret;
    }

    if (!sys_status.LDEDONE) {
        ret.err_code = DW1000_RX_ERROR_TIMESTAMP_PENDING;
        return ret;
    }

    // Read RXPACC_NOSAT
    dw1000_read(instance, 0x27, 0x2C, sizeof(uint16_t), &rxpacc_nosat);

    // Read RX_FQUAL
    dw1000_read(instance, DW1000_RX_FRAME_QUALITY_INFORMATION_FILE, 0, sizeof(rx_fqual), &rx_fqual);

    // Read FP_AMPL1
    dw1000_read(instance, 0x15, 0x07, sizeof(uint16_t), &fp_ampl1);

    // Read RX_TIME
    dw1000_read(instance, 0x15, 0, 5, &ret.timestamp);
    dw1000_read(instance, 0x13, 0, 4, &ret.rx_ttcki);
    dw1000_read(instance, 0x14, 0, 4, &ret.rx_ttcko);

    // Read RX_FINFO
    dw1000_read(instance, DW1000_RX_FRAME_INFORMATION_REGISTER_FILE, 0, sizeof(rx_finfo), &rx_finfo);
    ret.len = (((uint16_t)rx_finfo.RXFLEN) | (((uint16_t)rx_finfo.RXFLE) << 7)) - 2;
    // Check if the frame fits in the provided buffer
    if (ret.len > 0 && ret.len <= buf_len) {
        // Read RX_BUFFER
        dw1000_read(instance, DW1000_RX_FRAME_BUFFER_FILE, 0, ret.len, buf);
    } else {
        ret.err_code = DW1000_RX_ERROR_PROVIDED_BUFFER_TOO_SMALL;
    }

    ret.rx_ttcko &= ((1<<19)-1);
    if (((ret.rx_ttcko>>18)&1) != 0) {
        // extend the sign bit
        ret.rx_ttcko |= ~((1<<19)-1);
    }

    rxpacc = rx_finfo.RXPACC;
    fp_ampl2 = rx_fqual.FP_AMPL2;
    fp_ampl3 = rx_fqual.FP_AMPL3;
    cir_pwr = rx_fqual.CIR_PWR;
    if (rxpacc_nosat == rxpacc) {
        rxpacc_corrected = rxpacc + dw1000_conf_get_rxpacc_correction(instance->config);
    } else {
        rxpacc_corrected = rxpacc;
    }

    // Compute rssi
    ret.fp_ampl1 = fp_ampl1;
    ret.fp_ampl2 = fp_ampl2;
    ret.fp_ampl3 = fp_ampl3;
    ret.cir_pwr = cir_pwr;
    ret.std_noise = rx_fqual.STD_NOISE;
    ret.rxpacc_corrected = rxpacc_corrected;
    ret.rssi_est = dw1000_get_rssi_est(instance, cir_pwr, rxpacc_corrected);
    ret.fp_rssi_est = dw1000_get_fp_rssi_est(instance, fp_ampl1, fp_ampl2, fp_ampl3, rxpacc_corrected);

    // Correct timestamp
    ret.timestamp = dw1000_correct_tstamp(instance, ret.rssi_est, ret.timestamp);

    // Read SYS_STATUS
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);
    // Check RXOVRR Again, We might have received a packet while we were reading data
    if (sys_status.RXOVRR) {
        // Frames must be discarded (do not read frames) due to corrupted registers and TRXOFF command issued
        dw1000_disable_transceiver(instance);
        dw1000_swap_rx_buffers(instance);
        dw1000_rx_softreset(instance);
    
        // Receiver must be reset to exit errored state
        dw1000_rx_enable(instance);

        memset(&ret,0,sizeof(ret));
        ret.err_code = DW1000_RX_ERROR_RXOVRR;
        return ret;
    }

    // Check HSRBP==ICRBP
    if (sys_status.HSRBP == sys_status.ICRBP) {
        // Mask, clear and unmask RX event flags in SYS_STATUS reg:0F; bits FCE, FCG, DFR, LDE_DONE
        dw1000_clear_double_buffered_status_bits(instance);
    }

    // Issue the HRBPT command
    dw1000_swap_rx_buffers(instance);

    return ret;
}

struct dw1000_rx_frame_info_s dw1000_receive_data_only(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf)
{
    struct dw1000_rx_frame_info_s ret;
    memset(&ret,0,sizeof(ret));
    if (!instance || !buf) {
        ret.err_code = DW1000_RX_ERROR_NULLPTR;
        return ret;
    }

    struct dw1000_sys_status_s sys_status;
    struct dw1000_rx_finfo_s rx_finfo;
    
    // Read SYS_STATUS
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Check RXOVRR
    if (sys_status.RXOVRR || sys_status.LDEERR || sys_status.RXFCE || sys_status.RXPTO || sys_status.RXSFDTO || sys_status.RXPHE || sys_status.RXRFSL || sys_status.RXRFTO) {
        // Frames must be discarded (do not read frames) due to corrupted registers and TRXOFF command issued
        dw1000_disable_transceiver(instance);
        dw1000_swap_rx_buffers(instance);
        dw1000_rx_softreset(instance);
    
        // Receiver must be reset to exit errored state
        dw1000_rx_enable(instance);

        memset(&ret,0,sizeof(ret));
        ret.err_code = DW1000_RX_ERROR_RXOVRR;
        return ret;
    }

    // Check if a good frame is in the buffer
    if (!sys_status.RXFCG) {
        ret.err_code = DW1000_RX_ERROR_NO_FRAME_PRESENT;
        return ret;
    }

    if (!sys_status.LDEDONE) {
        ret.err_code = DW1000_RX_ERROR_TIMESTAMP_PENDING;
        return ret;
    }

    // Read RX_FINFO
    dw1000_read(instance, DW1000_RX_FRAME_INFORMATION_REGISTER_FILE, 0, sizeof(rx_finfo), &rx_finfo);
    ret.len = (((uint16_t)rx_finfo.RXFLEN) | (((uint16_t)rx_finfo.RXFLE) << 7)) - 2;
    // Check if the frame fits in the provided buffer
    if (ret.len > 0 && ret.len <= buf_len) {
        // Read RX_BUFFER
        dw1000_read(instance, DW1000_RX_FRAME_BUFFER_FILE, 0, ret.len, buf);
    } else {
        ret.err_code = DW1000_RX_ERROR_PROVIDED_BUFFER_TOO_SMALL;
    }

    // Read SYS_STATUS
    dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);
    // Check RXOVRR Again, We might have received a packet while we were reading data
    if (sys_status.RXOVRR) {
        // Frames must be discarded (do not read frames) due to corrupted registers and TRXOFF command issued
        dw1000_disable_transceiver(instance);
        dw1000_swap_rx_buffers(instance);
        dw1000_rx_softreset(instance);
    
        // Receiver must be reset to exit errored state
        dw1000_rx_enable(instance);

        memset(&ret,0,sizeof(ret));
        ret.err_code = DW1000_RX_ERROR_RXOVRR;
        return ret;
    }
    // Check HSRBP==ICRBP
    if (sys_status.HSRBP == sys_status.ICRBP) {
        // Mask, clear and unmask RX event flags in SYS_STATUS reg:0F; bits FCE, FCG, DFR, LDE_DONE
        dw1000_clear_double_buffered_status_bits(instance);
    }

    // Issue the HRBPT command
    dw1000_swap_rx_buffers(instance);
    return ret;
}

void dw1000_transmit(struct dw1000_instance_s* instance, uint32_t buf_len, void* buf, bool expect_response) {
    if (!instance || !buf) {
        return;
    }

    if ((buf_len > 1021 && !instance->config.std_data_length) || 
        (buf_len > 125 && instance->config.std_data_length)) {
        return;
    }

    //Turn off Transceiver
    {
        struct dw1000_sys_ctrl_s sys_ctrl;
        // Issue TRXOFF
        memset(&sys_ctrl, 0, sizeof(sys_ctrl));
        sys_ctrl.TRXOFF = 1;
        dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
    }

    //Clear RX Status Bits
    {
        uint32_t sys_status =  DW1000_IRQ_MASK(DW1000_SYS_STATUS_RXFCE) |
                                  DW1000_IRQ_MASK(DW1000_SYS_STATUS_RXFCG) |
                                  DW1000_IRQ_MASK(DW1000_SYS_STATUS_RXDFR) |
                                  DW1000_IRQ_MASK(DW1000_SYS_STATUS_LDEDONE);
        dw1000_write(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);
    }

    // Write tx data to data buffer
    dw1000_write(instance, DW1000_TRANSMIT_DATA_BUFFER_FILE, 0, buf_len, buf);

    // Start tx
    {
        struct dw1000_sys_ctrl_s sys_ctrl;
        memset(&sys_ctrl, 0, sizeof(sys_ctrl));
        sys_ctrl.TXSTRT = 1;
        if (expect_response) {
            sys_ctrl.WAIT4RESP = 1;
        } 
        dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
    }

    // Configure tx parameters
    {
        struct dw1000_tx_fctrl_s tx_fctrl = dw1000_conf_tx_fctrl(instance->config);
        tx_fctrl.reserved = 0;
        tx_fctrl.TFLEN = (buf_len+2) & 0x7F;
        tx_fctrl.TFLE = ((buf_len+2) >> 7) & 0x7;
        dw1000_write(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);
    }

    //chHeapFree(data);
}

bool dw1000_scheduled_transmit(struct dw1000_instance_s* instance, uint64_t transmit_time, uint32_t buf_len, void* buf, bool expect_response) {
    if (!instance) {
        return false;
    }

    if ((buf_len > 1021 && !instance->config.std_data_length) || 
        (buf_len > 125 && instance->config.std_data_length)) {
        return false;
    }

    // Write tx data to data buffer
    dw1000_write(instance, DW1000_TRANSMIT_DATA_BUFFER_FILE, 0, buf_len, buf);

    // Configure tx parameters
    {
        struct dw1000_tx_fctrl_s tx_fctrl;
        dw1000_read(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);
        tx_fctrl.TFLEN = (buf_len+2) & 0x7F;
        tx_fctrl.TFLE = ((buf_len+2) >> 7) & 0x7;
        dw1000_write(instance, DW1000_TRANSMIT_FRAME_CONTROL_FILE, 0, sizeof(tx_fctrl), &tx_fctrl);
    }

    // Write transmission time
    dw1000_write(instance, 0x0A, 0, 5, &transmit_time);

    // Schedule tx
    {
        struct dw1000_sys_ctrl_s sys_ctrl;
        memset(&sys_ctrl, 0, sizeof(sys_ctrl));
        sys_ctrl.TXSTRT = 1;
        sys_ctrl.TXDLYS = 1;
        if (expect_response) {
            sys_ctrl.WAIT4RESP = 1;
        }
        dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
    }

    // Check for HPDWARN
    {
        struct dw1000_sys_status_s sys_status;
        dw1000_read(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);
        if (sys_status.HPDWARN) {
            dw1000_disable_transceiver(instance);
            if (expect_response) {
                dw1000_rx_enable(instance);
            }
            return false;
        }
    }
    return true;
}

uint64_t dw1000_get_tx_stamp(struct dw1000_instance_s* instance) {
    uint64_t ret = 0;
    dw1000_read(instance, 0x17, 0, 5, &ret);
    return ret;
}

int64_t dw1000_get_sys_time(struct dw1000_instance_s* instance) {
    int64_t ret = 0;
    dw1000_read(instance, 0x06, 0, 5, &ret);
    return ret;
}

systime_t dw1000_ticks_to_systicks(uint64_t dw1000_ticks) {
    const uint32_t attoseconds_per_dw1000_tick = 15650040;
    const uint64_t attoseconds_per_systick = 1000000000000000000/CH_CFG_ST_FREQUENCY;

    return (systime_t)((dw1000_ticks*attoseconds_per_dw1000_tick)/attoseconds_per_systick);
}

uint64_t systicks_to_dw1000_ticks(systime_t systicks) {
    const uint32_t attoseconds_per_dw1000_tick = 15650040;
    const uint64_t attoseconds_per_systick = 1000000000000000000/CH_CFG_ST_FREQUENCY;

    return ((uint64_t)systicks*attoseconds_per_systick)/attoseconds_per_dw1000_tick;
}

systime_t dw1000_timestamp_to_systime(struct dw1000_instance_s* instance, uint64_t dw1000_timestamp) {
    // TODO ideally do these together in a critical section
    uint64_t dw1000_tnow = dw1000_get_sys_time(instance);
    systime_t tnow_systicks = chVTGetSystemTimeX();

    systime_t tdiff_systicks = dw1000_ticks_to_systicks(dw1000_wrap_timestamp(dw1000_tnow-dw1000_timestamp));
    return tnow_systicks-tdiff_systicks;
}

uint16_t dw1000_get_ant_delay(struct dw1000_instance_s* instance) {
    uint16_t ret = 0;
    dw1000_read(instance, 0x18, 0, 2, &ret);
    return ret;
}


void dw1000_set_ant_delay(struct dw1000_instance_s* instance, uint16_t ant_delay)
{
    instance->config.ant_delay = ant_delay;
    // [0x00:0x01] TX_ANTD
    dw1000_write16(instance, 0x18, 0, ant_delay);
    // [0x1804:0x1805] LDE_RXANTD
    dw1000_write16(instance, 0x2E, 0x1804, ant_delay);    
}

float dw1000_get_temp(struct dw1000_instance_s* instance) {
    dw1000_write8(instance, 0x28, 0x11, 0x80);
    dw1000_write8(instance, 0x28, 0x12, 0x0A);
    dw1000_write8(instance, 0x28, 0x12, 0x0F);
    dw1000_write8(instance, 0x2A, 0x00, 0x01);
    chThdSleep(LL_US2ST(10));
    dw1000_write8(instance, 0x2A, 0x00, 0x00);
    uint8_t sarl_value[2] = {};
    dw1000_read(instance, 0x2A, 0x03, 2, sarl_value);

    return ((int8_t)(sarl_value[1] - instance->t_meas_23c)) * 1.14f + 23;
}

void dw1000_set_tx_gain_dB(struct dw1000_instance_s* instance, float tx_power_dB) {
    tx_power_dB = constrain_float(tx_power_dB, 0, 33.5);

    uint8_t coarse_dB = tx_power_dB < 18 ? (uint8_t)(tx_power_dB/3)*3 : 18;
    uint8_t fine_dB = roundf((tx_power_dB-coarse_dB)*2);
    uint8_t tx_power = (2*fine_dB) | (0b110-coarse_dB/3)<<5;

    instance->config.tx_power = tx_power;
    // [0x00:0x03] TX_POWER
    dw1000_write32(instance, 0x1E, 0x00, instance->config.tx_power<<24 |
                                        instance->config.tx_power<<16 |
                                        instance->config.tx_power<<8  |
                                        instance->config.tx_power);
}

static void dw1000_clock_force_sys_xti(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    uint8_t reg[2];
    spi_device_set_max_speed_hz(&instance->spi_dev, 3000000);
    dw1000_write8(instance, 0x36, 0, 0x01 | (reg[0]&0xfc));
    dw1000_write8(instance, 0x36, 1, reg[1]);
    chThdSleepMicroseconds(10);
}

static void dw1000_clock_enable_all_seq(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    uint8_t reg[2];
    spi_device_set_max_speed_hz(&instance->spi_dev, 3000000);
    dw1000_read(instance, 0x36, 0, 2, reg);
    dw1000_write8(instance, 0x36, 0, 0x00);
    dw1000_write8(instance, 0x36, 1, reg[1]&0xfe);
    spi_device_set_max_speed_hz(&instance->spi_dev, 20000000);
    chThdSleepMicroseconds(10);
}

static void dw1000_load_lde_microcode(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    // DW1000 User Manual Section 2.5.5.10
    dw1000_write16(instance, 0x36, 0x00, 0x0301);
    spi_device_set_max_speed_hz(&instance->spi_dev, 3000000);
    dw1000_write16(instance, 0x2D, 0x06, 0x8000);
    chThdSleepMicroseconds(150);
    dw1000_write16(instance, 0x36, 0x00, 0x0200);
    spi_device_set_max_speed_hz(&instance->spi_dev, 20000000);
}

static void dw1000_load_ldotune(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    // DW1000 User Manual Section 2.5.5.11
    uint8_t ldotune_cal[5];
    dw1000_otp_read(instance, 0x04, 4, &ldotune_cal[0]);
    dw1000_otp_read(instance, 0x05, 1, &ldotune_cal[4]);
    if (ldotune_cal[0] != 0) {
        dw1000_write16(instance, 0x2C, 0x00, (1<<11)|(1<<12));
        dw1000_write(instance, 0x28, 0x30, sizeof(ldotune_cal), ldotune_cal);
    } else {
        dw1000_write16(instance, 0x2C, 0x00, (1<<11));
    }
}

static void dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(struct dw1000_instance_s* instance, bool transceiver_disable) {
    if (!instance) {
        return;
    }

    struct dw1000_sys_mask_s sys_mask_prev;
    struct dw1000_sys_mask_s sys_mask;
    struct dw1000_sys_ctrl_s sys_ctrl;
    struct dw1000_sys_status_s sys_status;

    // Read SYS_MASK
    dw1000_read(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(sys_mask), &sys_mask);
    sys_mask_prev = sys_mask;

    // Mask double buffered status bits FCE, FCG, DFR, LDE_DONE
    sys_mask.MRXFCE = 0;
    sys_mask.MRXFCG = 0;
    sys_mask.MRXDFR = 0;
    sys_mask.MLDEDONE = 0;

    // Write SYS_MASK
    dw1000_write(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(sys_mask), &sys_mask);

    if (transceiver_disable) {
        // Issue TRXOFF
        memset(&sys_ctrl, 0, sizeof(sys_ctrl));
        sys_ctrl.TRXOFF = 1;

        // Write SYS_CTRL
        dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
    }

    // Clear double buffered status bits FCE, FCG, DFR, LDE_DONE
    memset(&sys_status, 0, sizeof(sys_status));
    sys_status.RXFCE = 1;
    sys_status.RXFCG = 1;
    sys_status.RXDFR = 1;
    sys_status.LDEDONE = 1;

    // Write SYS_STATUS
    dw1000_write(instance, DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE, 0, sizeof(sys_status), &sys_status);

    // Unmask double buffered status bits FCE, FCG, DFR, LDE_DONE
    sys_mask = sys_mask_prev;

    // Write SYS_MASK
    dw1000_write(instance, DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE, 0, sizeof(sys_mask), &sys_mask);
}

static void dw1000_clear_double_buffered_status_bits(struct dw1000_instance_s* instance) {
    dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(instance, false);
}

void dw1000_disable_transceiver(struct dw1000_instance_s* instance) {
    dw1000_clear_double_buffered_status_bits_and_optionally_disable_transceiver(instance, true);
}

void dw1000_swap_rx_buffers(struct dw1000_instance_s* instance) {
    // Issue the HRBPT command
    struct dw1000_sys_ctrl_s sys_ctrl;
    memset(&sys_ctrl,0,sizeof(sys_ctrl));
    sys_ctrl.HRBPT = 1;
    dw1000_write(instance, DW1000_SYSTEM_CONTROL_REGISTER_FILE, 0, sizeof(sys_ctrl), &sys_ctrl);
}

static void dw1000_hard_reset(struct dw1000_instance_s* instance) {
    if (!instance) {
        return;
    }

    palClearLine(instance->reset_line);
    palSetLineMode(instance->reset_line, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    chThdSleepMilliseconds(2);
    palSetLineMode(instance->reset_line, 0);
    chThdSleepMilliseconds(2);
}

static void dw1000_write8(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint8_t val) {
    dw1000_write(instance, regfile, reg, sizeof(val), &val);
}

static void dw1000_write16(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint16_t val) {
    dw1000_write(instance, regfile, reg, sizeof(val), &val);
}

static void dw1000_write32(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t val) {
    dw1000_write(instance, regfile, reg, sizeof(val), &val);
}

static void dw1000_write(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, const void* buf) {
    if (!instance || !buf) {
        return;
    }

    struct __attribute__((packed)) dw1000_transaction_header_s header;
    header.subidx_present = 1;
    header.write = 1;
    header.extended_address = 1;
    header.regfile = regfile;
    header.addressl = reg;
    header.addressh = reg>>7;
    if (len <= 4) {
        struct __attribute__((packed)) pkt_s {
            struct __attribute__((packed)) dw1000_transaction_header_s header;
            uint8_t data[4];
        } pkt;
        pkt.header = header;
        for (uint8_t i = 0; i < len; i++) {
            pkt.data[i] = ((uint8_t*)buf)[i];
        }
        spi_device_begin(&instance->spi_dev);
        spi_device_send(&instance->spi_dev, sizeof(struct dw1000_transaction_header_s) + len, &pkt);
        spi_device_end(&instance->spi_dev);
        return;
    }
    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(header), &header);
    spi_device_send(&instance->spi_dev, len, buf);
    spi_device_end(&instance->spi_dev);
}

static void dw1000_read(struct dw1000_instance_s* instance, uint8_t regfile, uint16_t reg, uint32_t len, void* buf) {
    if (!instance || !buf) {
        return;
    }

    struct __attribute__((packed)) dw1000_transaction_header_s header;
    header.subidx_present = 1;
    header.write = 0;
    header.extended_address = 1;
    header.regfile = regfile;
    header.addressl = reg;
    header.addressh = reg>>7;

    spi_device_begin(&instance->spi_dev);
    spi_device_send(&instance->spi_dev, sizeof(header), &header);
    spi_device_receive(&instance->spi_dev, len, buf);
    spi_device_end(&instance->spi_dev);
}

static void dw1000_otp_read(struct dw1000_instance_s* instance, uint16_t otp_addr, uint8_t otp_field_len, void* buf) {
    if (!instance || !buf) {
        return;
    }

    // NOTE: Per DW1000 API code, system clock needs to be XTI - this is necessary to make sure the values read from OTP are reliable

    // DW1000 User Manual Section 6.3.3
    dw1000_write16(instance, 0x2D, 0x04, otp_addr);
    dw1000_write8(instance, 0x2D, 0x06, 0x03);
    dw1000_write8(instance, 0x2D, 0x06, 0x01);
    dw1000_read(instance, 0x2D, 0x0A, otp_field_len, buf);
    dw1000_write8(instance, 0x2D, 0x06, 0x00);
}

void enable_sniff_mode(struct dw1000_instance_s* instance, uint8_t on_time_pac, uint8_t off_time_pac)
{
    switch (instance->config.preamble) {
        case DW1000_PREAMBLE_64:
        case DW1000_PREAMBLE_128:
            // PAC size 8
            off_time_pac *= 8;
            if((off_time_pac) > 0xFF) {
                off_time_pac = 0xFF;
            }
            dw1000_write16(instance, 0x1D, 0x00, (((off_time_pac) << 8) | on_time_pac));
        case DW1000_PREAMBLE_256:
        case DW1000_PREAMBLE_512:
            // PAC size 16
            off_time_pac *= 16;
            if((off_time_pac) > 0xFF) {
                off_time_pac = 0xFF;
            }
            dw1000_write16(instance, 0x1D, 0x00, (((off_time_pac) << 8) | on_time_pac));
        case DW1000_PREAMBLE_1024:
            // PAC size 32
            off_time_pac *= 32;
            if((off_time_pac) > 0xFF) {
                off_time_pac = 0xFF;
            }
            dw1000_write16(instance, 0x1D, 0x00, (((off_time_pac) << 8) | on_time_pac));
        case DW1000_PREAMBLE_1536:
        case DW1000_PREAMBLE_2048:
        case DW1000_PREAMBLE_4096:
            // PAC size 64
            off_time_pac *= 64;
            if((off_time_pac) > 0xFF) {
                off_time_pac = 0xFF;
            }
            dw1000_write16(instance, 0x1D, 0x00, (((off_time_pac) << 8) | on_time_pac));
    }
}

void disable_sniff_mode(struct dw1000_instance_s* instance)
{
    dw1000_write16(instance, 0x1D, 0x00, 0x00);
}

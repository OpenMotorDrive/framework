#pragma once

#include "dw1000.h"
#include <string.h>

#define DW1000_SYSTEM_CONFIGURATION_FILE 0x04
#define DW1000_TRANSMIT_FRAME_CONTROL_FILE 0x08
#define DW1000_TRANSMIT_DATA_BUFFER_FILE 0x09
#define DW1000_SYSTEM_CONTROL_REGISTER_FILE 0x0D
#define DW1000_RX_FRAME_INFORMATION_REGISTER_FILE 0x10
#define DW1000_RX_FRAME_BUFFER_FILE 0x11
#define DW1000_SYSTEM_EVENT_MASK_REGISTER_FILE 0x0E
#define DW1000_SYSTEM_EVENT_STATUS_REGISTER_FILE 0x0F
#define DW1000_OTP_MEMORY_INTERFACE_FILE 0x2D
#define DW1000_POWER_MANAGEMENT_AND_SYSTEM_CONTROL_FILE 0x36

struct __attribute__((packed)) dw1000_rx_finfo_s {
    struct { // bytes [0:3]
        uint32_t RXFLEN:7;           // [0:6]
        uint32_t RXFLE:3;            // [7:9]
        uint32_t reserved1:1;        // [10]
        uint32_t RXNSPL:2;           // [11:12]
        uint32_t RXBR:2;             // [13:14]
        uint32_t RNG:1;              // [15]
        uint32_t RXPRF:2;            // [16:17]
        uint32_t RXPSR:2;            // [18:19]
        uint32_t RXPACC:12;          // [20:31]
    };
};

struct __attribute__((packed)) dw1000_transaction_header_s {
    struct {
        uint8_t regfile:6;            // [0:5]
        uint8_t subidx_present:1;     // [6]
        uint8_t write:1;              // [7]
    };
    struct {
        uint8_t addressl:7;           // [0:6]
        uint8_t extended_address:1;   // [7]
    };
    struct {
        uint8_t addressh;             // [0:7]
    };
};

struct __attribute__((packed)) dw1000_sys_cfg_s {
    struct { // bytes [0:3]
        uint32_t FFEN:1;              // [0]
        uint32_t FFBC:1;              // [1]
        uint32_t FFAB:1;              // [2]
        uint32_t FFAD:1;              // [3]
        uint32_t FFAA:1;              // [4]
        uint32_t FFAM:1;              // [5]
        uint32_t FFAR:1;              // [6]
        uint32_t FFA4:1;              // [7]
        uint32_t FFA5:1;              // [8]
        uint32_t HIRQ_POL:1;          // [9]
        uint32_t SPI_EDGE:1;          // [10]
        uint32_t DIS_FCE:1;           // [11]
        uint32_t DIS_DRXB:1;          // [12]
        uint32_t DIS_PHE:1;           // [13]
        uint32_t DIS_RSDE:1;          // [14]
        uint32_t FCS_INIT2F:1;        // [15]
        uint32_t PHR_MODE:2;          // [16:17]
        uint32_t DIS_STXP:1;          // [18]
        uint32_t reserved1:3;         // [19:21]
        uint32_t RXM110K:1;           // [22]
        uint32_t reserved2:5;         // [23:27]
        uint32_t RXWTOE:1;            // [28]
        uint32_t RXAUTR:1;            // [29]
        uint32_t AUTOACK:1;           // [30]
        uint32_t AACKPEND:1;          // [31]
    };
};

struct __attribute__((packed)) dw1000_sys_mask_s {
    struct { // bytes [0:3]
        uint32_t reserved1:1;          // [0]
        uint32_t MCPLOCK:1;            // [1]
        uint32_t MESYNCR:1;            // [2]
        uint32_t MAAT:1;               // [3]
        uint32_t MTXFRB:1;             // [4]
        uint32_t MTXPRS:1;             // [5]
        uint32_t MTXPHS:1;             // [6]
        uint32_t MTXFRS:1;             // [7]
        uint32_t MRXPRD:1;             // [8]
        uint32_t MRXSFDD:1;            // [9]
        uint32_t MLDEDONE:1;           // [10]
        uint32_t MRXPHD:1;             // [11]
        uint32_t MRXPHE:1;             // [12]
        uint32_t MRXDFR:1;             // [13]
        uint32_t MRXFCG:1;             // [14]
        uint32_t MRXFCE:1;             // [15]
        uint32_t MRXRFSL:1;            // [16]
        uint32_t MRXRFTO:1;            // [17]
        uint32_t MLDEERR:1;            // [18]
        uint32_t reserved2:1;          // [19]
        uint32_t MRXOVRR:1;            // [20]
        uint32_t MRXPTO:1;             // [21]
        uint32_t MGPIOIRQ:1;           // [22]
        uint32_t MSLP2INIT:1;          // [23]
        uint32_t MRFPLL_LL:1;          // [24]
        uint32_t MCLKPLL_LL:1;         // [25]
        uint32_t MRXSFDTO:1;           // [26]
        uint32_t MHPDWARN:1;           // [27]
        uint32_t MTXBERR:1;            // [28]
        uint32_t MAFFREJ:1;            // [29]
        uint32_t reserved3:2;          // [30:31]
    };
};

struct __attribute__((packed)) dw1000_sys_status_s {
    struct { // bytes [0:3]
        uint32_t IRQS:1;               // [0]
        uint32_t CPLOCK:1;             // [1]
        uint32_t ESYNCR:1;             // [2]
        uint32_t AAT:1;                // [3]
        uint32_t TXFRB:1;              // [4]
        uint32_t TXPRS:1;              // [5]
        uint32_t TXPHS:1;              // [6]
        uint32_t TXFRS:1;              // [7]
        uint32_t RXPRD:1;              // [8]
        uint32_t RXSFDD:1;             // [9]
        uint32_t LDEDONE:1;            // [10]
        uint32_t RXPHD:1;              // [11]
        uint32_t RXPHE:1;              // [12]
        uint32_t RXDFR:1;              // [13]
        uint32_t RXFCG:1;              // [14]
        uint32_t RXFCE:1;              // [15]
        uint32_t RXRFSL:1;             // [16]
        uint32_t RXRFTO:1;             // [17]
        uint32_t LDEERR:1;             // [18]
        uint32_t reserved1:1;          // [19]
        uint32_t RXOVRR:1;             // [20]
        uint32_t RXPTO:1;              // [21]
        uint32_t GPIOIRQ:1;            // [22]
        uint32_t SLP2INIT:1;           // [23]
        uint32_t RFPLL_LL:1;           // [24]
        uint32_t CLKPLL_LL:1;          // [25]
        uint32_t RXSFDTO:1;            // [26]
        uint32_t HPDWARN:1;            // [27]
        uint32_t TXBERR:1;             // [28]
        uint32_t AFFREJ:1;             // [29]
        uint32_t HSRBP:1;              // [30]
        uint32_t ICRBP:1;              // [31]
    };
    struct { // byte [4]
        uint8_t RXRSCS:1;              // [0]
        uint8_t RXPREJ:1;              // [1]
        uint8_t TXPUTE:1;              // [2]
        uint8_t reserved2:5;           // [3:7]
    };
};

struct __attribute__((packed)) dw1000_sys_ctrl_s {
    struct { // bytes [0:3]
        uint32_t SFCST:1;              // [0]
        uint32_t TXSTRT:1;             // [1]
        uint32_t TXDLYS:1;             // [2]
        uint32_t CANSFCS:1;            // [3]
        uint32_t reserved1:2;          // [4:5]
        uint32_t TRXOFF:1;             // [6]
        uint32_t WAIT4RESP:1;          // [7]
        uint32_t RXENAB:1;             // [8]
        uint32_t RXDLYE:1;             // [9]
        uint32_t reserved2:14;         // [10:23]
        uint32_t HRBPT:1;              // [24]
        uint32_t reserved4:7;          // [25:31]
    };
};

struct __attribute__((packed)) dw1000_tx_fctrl_s {
    struct { // register 08:00
        uint32_t TFLEN:7;              // [0:6]
        uint32_t TFLE:3;               // [7:9]
        uint32_t reserved:3;           // [10:12]
        uint32_t TXBR:2;               // [13:14]
        uint32_t TR:1;                 // [15]
        uint32_t TXPRF:2;              // [16:17]
        uint32_t TXPSR:2;              // [18:19]
        uint32_t PE:2;                 // [20:21]
        uint32_t TXBOFFS:10;           // [22:31]
    };
    struct { // register 08:04
        uint8_t IFSDELAY:8;            // [0:7]
    };
};

struct __attribute__((packed)) dw1000_chan_ctrl_s {
    struct { // register 1F:00
        uint32_t TX_CHAN:4;            // [0:3]
        uint32_t RX_CHAN:4;            // [4:7]
        uint32_t reserved:9;           // [8:16]
        uint32_t DWSFD:1;              // [17]
        uint32_t RXPRF:2;              // [18:19]
        uint32_t TNSSFD:1;             // [20]
        uint32_t RNSSFD:1;             // [21]
        uint32_t TX_PCODE:5;           // [22:26]
        uint32_t RX_PCODE:5;           // [27:31]
    };
};

static uint8_t dw1000_conf_get_default_pcode(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            switch (config.channel) {
                case DW1000_CHANNEL_1:
                    return 1;
                case DW1000_CHANNEL_2:
                case DW1000_CHANNEL_5:
                    return 3;
                case DW1000_CHANNEL_3:
                    return 5;
                case DW1000_CHANNEL_4:
                case DW1000_CHANNEL_7:
                    return 7;
            }
            break;
        case DW1000_PRF_64MHZ:
            switch (config.channel) {
                case DW1000_CHANNEL_1:
                case DW1000_CHANNEL_2:
                case DW1000_CHANNEL_3:
                case DW1000_CHANNEL_5:
                    return 9;
                case DW1000_CHANNEL_4:
                case DW1000_CHANNEL_7:
                    return 17;
            }
            break;
    }
}

static uint16_t dw1000_conf_lde_repc(struct dw1000_config_s config) {
    struct {
        uint8_t pcode;
        uint16_t lde_repc;
    } map[] = {
        { 1, 0x5998},
        { 2, 0x5998},
        { 3, 0x51EA},
        { 4, 0x428E},
        { 5, 0x451E},
        { 6, 0x2E14},
        { 7, 0x8000},
        { 8, 0x51EA},
        { 9, 0x28F4},
        {10, 0x3332},
        {11, 0x3AE0},
        {12, 0x3D70},
        {13, 0x3AE0},
        {14, 0x35C2},
        {15, 0x2B84},
        {16, 0x35C2},
        {17, 0x3332},
        {18, 0x35C2},
        {19, 0x35C2},
        {20, 0x47AE},
        {21, 0x3AE0},
        {22, 0x3850},
        {23, 0x30A2},
        {24, 0x3850},
    };

    for (uint8_t i=0; i<sizeof(map)/sizeof(map[0]); i++) {
        if (map[i].pcode == config.pcode) {
            if (config.data_rate != DW1000_DATA_RATE_110K) {
                return map[i].lde_repc;
            } else {
                return map[i].lde_repc/8;
            }
        }
    }
    return 0;
}

static uint8_t dw1000_conf_dwsfd(struct dw1000_config_s config) {
    switch (config.data_rate) {
        case DW1000_DATA_RATE_110K:
            return 1;
        case DW1000_DATA_RATE_850K:
            return 1;
        case DW1000_DATA_RATE_6_8M:
            return 0;
    }
    return 0;
}

static uint8_t dw1000_conf_sfd_length(struct dw1000_config_s config) {
    switch (config.data_rate) {
        case DW1000_DATA_RATE_110K:
            return 16;
        case DW1000_DATA_RATE_850K:
            return 16;
        case DW1000_DATA_RATE_6_8M:
            return 8;
    }
    return 0;
}

static uint32_t dw1000_conf_fs_pllcfg(struct dw1000_config_s config) {
    switch (config.channel) {
        case DW1000_CHANNEL_1:
            return 0x09000407;
        case DW1000_CHANNEL_2:
        case DW1000_CHANNEL_4:
            return 0x08400508;
        case DW1000_CHANNEL_3:
            return 0x08401009;
        case DW1000_CHANNEL_5:
        case DW1000_CHANNEL_7:
            return 0x0800041D;
    }
    return 0;
}

static uint8_t dw1000_conf_fs_plltune(struct dw1000_config_s config) {
    switch (config.channel) {
        case DW1000_CHANNEL_1:
            return 0x1E;
        case DW1000_CHANNEL_2:
        case DW1000_CHANNEL_4:
            return 0x26;
        case DW1000_CHANNEL_3:
            return 0x56;
        case DW1000_CHANNEL_5:
        case DW1000_CHANNEL_7:
            return 0xBE;
    }
    return 0;
}

static uint8_t dw1000_conf_tc_pgdelay(struct dw1000_config_s config) {
    switch (config.channel) {
        case DW1000_CHANNEL_1:
            return 0xC9;
        case DW1000_CHANNEL_2:
            return 0xC2;
        case DW1000_CHANNEL_3:
            return 0xC5;
        case DW1000_CHANNEL_4:
            return 0x95;
        case DW1000_CHANNEL_5:
            return 0xC0;
        case DW1000_CHANNEL_7:
            return 0x93;
    }
    return 0;
}

static uint32_t dw1000_conf_rf_txctrl(struct dw1000_config_s config) {
    switch (config.channel) {
        case DW1000_CHANNEL_1:
            return 0x00005C40;
        case DW1000_CHANNEL_2:
            return 0x00045CA0;
        case DW1000_CHANNEL_3:
            return 0x00086CC0;
        case DW1000_CHANNEL_4:
            return 0x00045C80;
        case DW1000_CHANNEL_5:
            return 0x001E3FE0;
        case DW1000_CHANNEL_7:
            return 0x001E7DE0;
    }
    return 0;
}

static uint32_t dw1000_conf_tx_power(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            switch (config.channel) {
                case DW1000_CHANNEL_1:
                case DW1000_CHANNEL_2:
                    return 0x15355575;
                case DW1000_CHANNEL_3:
                    return 0x0F2F4F6F;
                case DW1000_CHANNEL_4:
                    return 0x1F1F3F5F;
                case DW1000_CHANNEL_5:
                    return 0x0E082848;
                case DW1000_CHANNEL_7:
                    return 0x32527292;
            }
            break;
        case DW1000_PRF_64MHZ:
            switch (config.channel) {
                case DW1000_CHANNEL_1:
                case DW1000_CHANNEL_2:
                    return 0x07274767;
                case DW1000_CHANNEL_3:
                    return 0x2B4B6B8B;
                case DW1000_CHANNEL_4:
                    return 0x3A5A7A9A;
                case DW1000_CHANNEL_5:
                    return 0x25456585;
                case DW1000_CHANNEL_7:
                    return 0x5171B1D1;
            }
            break;
    }
    return 0;
}

static uint16_t dw1000_conf_drx_tune0b(struct dw1000_config_s config) {
    bool dwsfd = dw1000_conf_dwsfd(config);
    switch (config.data_rate) {
        case DW1000_DATA_RATE_110K:
            return (!dwsfd)?0x000A:0x0016;
        case DW1000_DATA_RATE_850K:
            return (!dwsfd)?0x0001:0x0006;
        case DW1000_DATA_RATE_6_8M:
            return (!dwsfd)?0x0001:0x0002;
    }
    return 0;
}

static uint16_t dw1000_conf_drx_tune1a(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            return 0x0087;
        case DW1000_PRF_64MHZ:
            return 0x008D;
    }
    return 0;
}

static uint16_t dw1000_conf_drx_tune1b(struct dw1000_config_s config) {
    switch (config.preamble) {
        case DW1000_PREAMBLE_64:
            return 0x0010;
        case DW1000_PREAMBLE_128:
        case DW1000_PREAMBLE_256:
        case DW1000_PREAMBLE_512:
        case DW1000_PREAMBLE_1024:
            return 0x0020;
        case DW1000_PREAMBLE_1536:
        case DW1000_PREAMBLE_2048:
        case DW1000_PREAMBLE_4096:
            return 0x0064;
    }
    return 0;
}

static uint32_t dw1000_conf_drx_tune2(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            switch (config.preamble) {
                case DW1000_PREAMBLE_64:
                case DW1000_PREAMBLE_128:
                    // PAC size 8
                    return 0x311A002D;
                case DW1000_PREAMBLE_256:
                case DW1000_PREAMBLE_512:
                    // PAC size 16
                    return 0x331A0052;
                case DW1000_PREAMBLE_1024:
                    // PAC size 32
                    return 0x351A009A;
                case DW1000_PREAMBLE_1536:
                case DW1000_PREAMBLE_2048:
                case DW1000_PREAMBLE_4096:
                    // PAC size 64
                    return 0x371A011D;
            }
            break;
        case DW1000_PRF_64MHZ:
            switch (config.preamble) {
                case DW1000_PREAMBLE_64:
                case DW1000_PREAMBLE_128:
                    return 0x313B006B;
                case DW1000_PREAMBLE_256:
                case DW1000_PREAMBLE_512:
                    return 0x333B00BE;
                case DW1000_PREAMBLE_1024:
                    return 0x353B015E;
                case DW1000_PREAMBLE_1536:
                case DW1000_PREAMBLE_2048:
                case DW1000_PREAMBLE_4096:
                    return 0x373B0296;
            }
            break;
    }
    return 0;
}

static uint16_t dw1000_get_preamble_num_symbols(struct dw1000_config_s config) {
    switch (config.preamble) {
        case DW1000_PREAMBLE_64:
            return 64;
        case DW1000_PREAMBLE_128:
            return 128;
        case DW1000_PREAMBLE_256:
            return 256;
        case DW1000_PREAMBLE_512:
            return 512;
        case DW1000_PREAMBLE_1024:
            return 1024;
        case DW1000_PREAMBLE_1536:
            return 1536;
        case DW1000_PREAMBLE_2048:
            return 2048;
        case DW1000_PREAMBLE_4096:
            return 4096;
    }
    return 0;
}

static uint16_t dw1000_conf_drx_sfdtoc(struct dw1000_config_s config) {
    uint16_t ret = 1;
    ret += dw1000_get_preamble_num_symbols(config);
    switch (config.data_rate) {
        case DW1000_DATA_RATE_110K:
            ret += 64;
            break;
        case DW1000_DATA_RATE_850K:
            ret += 16;
            break;
        case DW1000_DATA_RATE_6_8M:
            ret += 8;
            break;
    }
    return ret;
}

static uint16_t dw1000_conf_drx_tune4h(struct dw1000_config_s config) {
    switch (config.preamble) {
        case DW1000_PREAMBLE_64:
            return 0x0010;
        case DW1000_PREAMBLE_128:
        case DW1000_PREAMBLE_256:
        case DW1000_PREAMBLE_512:
        case DW1000_PREAMBLE_1024:
        case DW1000_PREAMBLE_1536:
        case DW1000_PREAMBLE_2048:
        case DW1000_PREAMBLE_4096:
            return 0x0028;
    }
    return 0;
}

static uint8_t dw1000_conf_rf_rxctrlh(struct dw1000_config_s config) {
    switch (config.channel) {
        case DW1000_CHANNEL_1:
        case DW1000_CHANNEL_2:
        case DW1000_CHANNEL_3:
        case DW1000_CHANNEL_5:
            return 0xD8;
        case DW1000_CHANNEL_4:
        case DW1000_CHANNEL_7:
            return 0xBC;
    }
    return 0;
}

static uint16_t dw1000_conf_agc_tune1(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            return 0x8870;
        case DW1000_PRF_64MHZ:
            return 0x889B;
    }
    return 0;
}

static uint16_t dw1000_conf_lde_cfg2(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            return 0x1607;
        case DW1000_PRF_64MHZ:
            return 0x0607;
    }
    return 0;
}

static struct dw1000_chan_ctrl_s dw1000_conf_chan_ctrl(struct dw1000_config_s config) {
    struct dw1000_chan_ctrl_s ret;
    memset(&ret, 0, sizeof(ret));

    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            ret.RXPRF = 0b01;
            break;
        case DW1000_PRF_64MHZ:
            ret.RXPRF = 0b10;
            break;
    }

    ret.TX_CHAN = ret.RX_CHAN = config.channel;
    ret.TX_PCODE = ret.RX_PCODE = config.pcode;

    ret.DWSFD = dw1000_conf_dwsfd(config);
    ret.TNSSFD = 0;
    ret.RNSSFD = 0;

    return ret;
}

static struct dw1000_tx_fctrl_s dw1000_conf_tx_fctrl(struct dw1000_config_s config) {
    struct dw1000_tx_fctrl_s ret;
    memset(&ret, 0, sizeof(ret));
    ret.TXBR = 0b10;
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            ret.TXPRF = 0b01;
            break;
        case DW1000_PRF_64MHZ:
            ret.TXPRF = 0b10;
            break;
    }

    switch (config.preamble) {
        case DW1000_PREAMBLE_64:
            ret.TXPSR = 0b01;
            ret.PE =    0b00;
            break;
        case DW1000_PREAMBLE_128:
            ret.TXPSR = 0b01;
            ret.PE =    0b01;
            break;
        case DW1000_PREAMBLE_256:
            ret.TXPSR = 0b01;
            ret.PE =    0b10;
            break;
        case DW1000_PREAMBLE_512:
            ret.TXPSR = 0b01;
            ret.PE =    0b11;
            break;
        case DW1000_PREAMBLE_1024:
            ret.TXPSR = 0b10;
            ret.PE =    0b00;
            break;
        case DW1000_PREAMBLE_1536:
            ret.TXPSR = 0b10;
            ret.PE =    0b01;
            break;
        case DW1000_PREAMBLE_2048:
            ret.TXPSR = 0b10;
            ret.PE =    0b10;
            break;
        case DW1000_PREAMBLE_4096:
            ret.TXPSR = 0b11;
            ret.PE =    0b00;
            break;
    }

    return ret;
}

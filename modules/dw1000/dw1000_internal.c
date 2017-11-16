#include "dw1000_internal.h"

int8_t dw1000_conf_get_rxpacc_correction(struct dw1000_config_s config) {
    // Per DW1000 User Manual, Table 18: RXPACC Adjustments by SFD code

    if (!dw1000_conf_dwsfd(config)) {
        switch(dw1000_conf_sfd_length(config)) {
            case 8:
                return -5;
            case 64:
                return -64;
        }
    } else {
        switch(dw1000_conf_sfd_length(config)) {
            case 8:
                return -10;
            case 16:
                return -18;
            case 64:
                return -82;
        }
    }

    return 0;
}

uint8_t dw1000_conf_get_default_pcode(struct dw1000_config_s config) {
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
    return 0;
}

uint16_t dw1000_conf_lde_repc(struct dw1000_config_s config) {
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

uint8_t dw1000_conf_dwsfd(struct dw1000_config_s config) {
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

uint8_t dw1000_conf_sfd_length(struct dw1000_config_s config) {
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

uint32_t dw1000_conf_fs_pllcfg(struct dw1000_config_s config) {
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

uint8_t dw1000_conf_fs_plltune(struct dw1000_config_s config) {
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

uint8_t dw1000_conf_tc_pgdelay(struct dw1000_config_s config) {
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

uint32_t dw1000_conf_rf_txctrl(struct dw1000_config_s config) {
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

uint32_t dw1000_conf_tx_power(struct dw1000_config_s config) {
    const uint32_t tx_power_octet = 0b11000000;
    return (tx_power_octet<<8)|(tx_power_octet<<16);
//     switch (config.prf) {
//         case DW1000_PRF_16MHZ:
//             switch (config.channel) {
//                 case DW1000_CHANNEL_1:
//                 case DW1000_CHANNEL_2:
//                     return 0x15355575;
//                 case DW1000_CHANNEL_3:
//                     return 0x0F2F4F6F;
//                 case DW1000_CHANNEL_4:
//                     return 0x1F1F3F5F;
//                 case DW1000_CHANNEL_5:
//                     return 0x0E082848;
//                 case DW1000_CHANNEL_7:
//                     return 0x32527292;
//             }
//             break;
//         case DW1000_PRF_64MHZ:
//             switch (config.channel) {
//                 case DW1000_CHANNEL_1:
//                 case DW1000_CHANNEL_2:
//                     return 0x07274767;
//                 case DW1000_CHANNEL_3:
//                     return 0x2B4B6B8B;
//                 case DW1000_CHANNEL_4:
//                     return 0x3A5A7A9A;
//                 case DW1000_CHANNEL_5:
//                     return 0x25456585;
//                 case DW1000_CHANNEL_7:
//                     return 0x5171B1D1;
//             }
//             break;
//     }
//     return 0;
}

uint16_t dw1000_conf_drx_tune0b(struct dw1000_config_s config) {
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

uint16_t dw1000_conf_drx_tune1a(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            return 0x0087;
        case DW1000_PRF_64MHZ:
            return 0x008D;
    }
    return 0;
}

uint16_t dw1000_conf_drx_tune1b(struct dw1000_config_s config) {
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

uint32_t dw1000_conf_drx_tune2(struct dw1000_config_s config) {
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

uint16_t dw1000_get_preamble_num_symbols(struct dw1000_config_s config) {
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

uint16_t dw1000_conf_drx_sfdtoc(struct dw1000_config_s config) {
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

uint16_t dw1000_conf_drx_tune4h(struct dw1000_config_s config) {
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

uint8_t dw1000_conf_rf_rxctrlh(struct dw1000_config_s config) {
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

uint16_t dw1000_conf_agc_tune1(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            return 0x8870;
        case DW1000_PRF_64MHZ:
            return 0x889B;
    }
    return 0;
}

uint16_t dw1000_conf_lde_cfg2(struct dw1000_config_s config) {
    switch (config.prf) {
        case DW1000_PRF_16MHZ:
            return 0x1607;
        case DW1000_PRF_64MHZ:
            return 0x0607;
    }
    return 0;
}

struct dw1000_chan_ctrl_s dw1000_conf_chan_ctrl(struct dw1000_config_s config) {
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

struct dw1000_tx_fctrl_s dw1000_conf_tx_fctrl(struct dw1000_config_s config) {
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

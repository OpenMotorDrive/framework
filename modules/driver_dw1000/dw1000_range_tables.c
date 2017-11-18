
#include <stdio.h>
#include <stdlib.h>

#include "dw1000.h"
#include "dw1000_internal.h"
#include <math.h>
#include <common/helpers.h>

static const uint8_t BIAS_500_16_ZERO = 10;
static const uint8_t BIAS_500_64_ZERO = 8;
static const uint8_t BIAS_900_16_ZERO = 7;
static const uint8_t BIAS_900_64_ZERO = 7;

// range bias tables (500 MHz in [mm] and 900 MHz in [2mm] - to fit into bytes)
static const uint8_t BIAS_500_16[] = {198, 187, 179, 163, 143, 127, 109, 84, 59, 31, 0, 36, 65, 84, 97, 106, 110, 112};
static const uint8_t BIAS_500_64[] = {110, 105, 100, 93, 82, 69, 51, 27, 0, 21, 35, 42, 49, 62, 71, 76, 81, 86};
static const uint8_t BIAS_900_16[] = {137, 122, 105, 88, 69, 47, 25, 0, 21, 48, 79, 105, 127, 147, 160, 169, 178, 197};
static const uint8_t BIAS_900_64[] = {147, 133, 117, 99, 75, 50, 29, 0, 24, 45, 63, 76, 87, 98, 116, 122, 132, 142};

float dw1000_get_fp_rssi_est(struct dw1000_instance_s* instance, uint16_t fp_ampl1, uint16_t fp_ampl2, uint16_t fp_ampl3, uint16_t rxpacc) {
    float A = (instance->config.prf == DW1000_PRF_16MHZ) ? 113.77f : 121.74f;
    float F1 = fp_ampl1;
    float F2 = fp_ampl2;
    float F3 = fp_ampl3;
    float N = rxpacc;
    return 10.0f*log10f((SQ(F1) + SQ(F2) + SQ(F3)) / SQ(N)) - A;
}

float dw1000_get_rssi_est(struct dw1000_instance_s* instance, uint16_t cir_pwr, uint16_t rxpacc) {
    float A = (instance->config.prf == DW1000_PRF_16MHZ) ? 113.77f : 121.74f;
    float C = cir_pwr;
    float N = rxpacc;
    float twoPower17 = 131072;
    float estRxPwr = 10.0f*log10f((C*twoPower17) / SQ(N)) - A;

    if (estRxPwr > -88.0f) {
        // approximation of Fig. 22 in user manual for dbm correction
        float corrFac = (instance->config.prf == DW1000_PRF_16MHZ) ? 2.3334f : 1.1667f;
        estRxPwr += (estRxPwr+88)*corrFac;
    }
    return estRxPwr;
}

int64_t dw1000_correct_tstamp(struct dw1000_instance_s* instance, float estRxPwr, int64_t ts) {
    // base line dBm, which is -61, 2 dBm steps, total 18 data points (down to -95 dBm)
    float rxPowerBase     = -(estRxPwr+61.0f)*0.5f;
    int16_t   rxPowerBaseLow  = (int16_t)rxPowerBase; // TODO check type
    int16_t   rxPowerBaseHigh = rxPowerBaseLow+1; // TODO check type
    if(rxPowerBaseLow <= 0) {
        rxPowerBaseLow  = 0;
        rxPowerBaseHigh = 0;
    } else if(rxPowerBaseHigh >= 17) {
        rxPowerBaseLow  = 17;
        rxPowerBaseHigh = 17;
    }
    // select range low/high values from corresponding table
    int16_t rangeBiasHigh;
    int16_t rangeBiasLow;
    if(instance->config.channel == DW1000_CHANNEL_4 || instance->config.channel == DW1000_CHANNEL_7) {
        // 900 MHz receiver bandwidth
        if(instance->config.prf == DW1000_PRF_16MHZ) {
            rangeBiasHigh = (rxPowerBaseHigh < BIAS_900_16_ZERO ? -BIAS_900_16[rxPowerBaseHigh] : BIAS_900_16[rxPowerBaseHigh]);
            rangeBiasHigh <<= 1;
            rangeBiasLow  = (rxPowerBaseLow < BIAS_900_16_ZERO ? -BIAS_900_16[rxPowerBaseLow] : BIAS_900_16[rxPowerBaseLow]);
            rangeBiasLow <<= 1;
        } else if(instance->config.prf == DW1000_PRF_64MHZ) {
            rangeBiasHigh = (rxPowerBaseHigh < BIAS_900_64_ZERO ? -BIAS_900_64[rxPowerBaseHigh] : BIAS_900_64[rxPowerBaseHigh]);
            rangeBiasHigh <<= 1;
            rangeBiasLow  = (rxPowerBaseLow < BIAS_900_64_ZERO ? -BIAS_900_64[rxPowerBaseLow] : BIAS_900_64[rxPowerBaseLow]);
            rangeBiasLow <<= 1;
        } else {
            // TODO proper error handling
            return ts;
        }
    } else {
        // 500 MHz receiver bandwidth
        if(instance->config.prf == DW1000_PRF_16MHZ) {
            rangeBiasHigh = (rxPowerBaseHigh < BIAS_500_16_ZERO ? -BIAS_500_16[rxPowerBaseHigh] : BIAS_500_16[rxPowerBaseHigh]);
            rangeBiasLow  = (rxPowerBaseLow < BIAS_500_16_ZERO ? -BIAS_500_16[rxPowerBaseLow] : BIAS_500_16[rxPowerBaseLow]);
        } else if(instance->config.prf == DW1000_PRF_64MHZ) {
            rangeBiasHigh = (rxPowerBaseHigh < BIAS_500_64_ZERO ? -BIAS_500_64[rxPowerBaseHigh] : BIAS_500_64[rxPowerBaseHigh]);
            rangeBiasLow  = (rxPowerBaseLow < BIAS_500_64_ZERO ? -BIAS_500_64[rxPowerBaseLow] : BIAS_500_64[rxPowerBaseLow]);
        } else {
            // TODO proper error handling
            return ts;
        }
    }
    // linear interpolation of bias values
    float rangeBias = rangeBiasLow+(rxPowerBase-rxPowerBaseLow)*(rangeBiasHigh-rangeBiasLow);
    // range bias [mm] to timestamp modification value conversion
    // apply correction
    ts = dw1000_wrap_timestamp(ts - (int16_t)(rangeBias*METERS_TO_TIME*0.001f));

    return ts;
}

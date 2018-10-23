#pragma once

#include "driver_pmw3901mb.h"

// Registers
#define PMW3901MB_PRODUCT_ID                0x00
#define PMW3901MB_REVISION_ID               0x01
#define PMW3901MB_MOTION                    0x02
#define PMW3901MB_DELTA_X_L                 0x03
#define PMW3901MB_DELTA_X_H                 0x04
#define PMW3901MB_DELTA_Y_L                 0x05
#define PMW3901MB_DELTA_Y_H                 0x06
#define PMW3901MB_SQUAL                     0x07
#define PMW3901MB_RAWDATA_SUM               0x08
#define PMW3901MB_MAXIMUM_RAWDATA           0x09
#define PMW3901MB_MINIMUM_RAWDATA           0x0A
#define PMW3901MB_SHUTTER_LOWER             0x0B
#define PMW3901MB_SHUTTER_UPPER             0x0C
#define PMW3901MB_OBSERVATION               0x15
#define PMW3901MB_MOTION_BURST              0x16
#define PMW3901MB_POWER_UP_RESET            0x3A
#define PMW3901MB_SHUTDOWN                  0x3B
#define PMW3901MB_RAWDATA_GRAB              0x58
#define PMW3901MB_RAWDATA_GRAB_STATUS       0x59
#define PMW3901MB_INVERSE_PRODUCT_ID        0x5F

// Timing
#define PMW3901MB_MOT_RST_MS                50
#define PMW3901MB_WAKEUP_MS                 50
#define PMW3901MB_TSWW_US                   45
#define PMW3901MB_TSRAD_US                  35
#define PMW3901MB_TSR_US                    20

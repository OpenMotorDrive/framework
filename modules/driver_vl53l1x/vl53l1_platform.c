#include "vl53l1_platform.h"
#include "vl53l1_platform_log.h"
#include "vl53l1_api.h"
#include <modules/timing/timing.h>

#include <ch.h>

#define trace_print(level, ...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_PLATFORM, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)

#define trace_i2c(...) \
	_LOG_TRACE_PRINT(VL53L1_TRACE_MODULE_NONE, \
	VL53L1_TRACE_LEVEL_NONE, VL53L1_TRACE_FUNCTION_I2C, ##__VA_ARGS__)

VL53L1_Error VL53L1_GetTickCount(uint32_t *ptick_count_ms)
{

    /* Returns current tick count in [ms] */

    VL53L1_Error status  = VL53L1_ERROR_NONE;

    *ptick_count_ms = millis();

#ifdef VL53L1_LOG_ENABLE
    trace_print(
            VL53L1_TRACE_LEVEL_DEBUG,
            "VL53L1_GetTickCount() = %5u ms;\n",
    *ptick_count_ms);
#endif

    return status;
}

VL53L1_Error VL53L1_RdByte(VL53L1_DEV dev, uint16_t index, uint8_t *data) {
    uint8_t tx_buf[2] = {index >> 8, index&0xFF};
        
    chThdSleepMicroseconds(1300);
    if (i2cMasterTransmitTimeout(dev->bus, dev->i2c_address, tx_buf, sizeof(tx_buf), data, 1, TIME_INFINITE) != I2C_NO_ERROR) {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }
    
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WrByte(VL53L1_DEV dev, uint16_t index, uint8_t data) {
    
    return VL53L1_WriteMulti(dev, index, &data, 1);
}

VL53L1_Error VL53L1_ReadMulti(VL53L1_DEV dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t tx_buf[2] = {index >> 8, index&0xFF};
    
    chThdSleepMicroseconds(1300);
    if (i2cMasterTransmitTimeout(dev->bus, dev->i2c_address, tx_buf, sizeof(tx_buf), pdata, count, TIME_INFINITE) != I2C_NO_ERROR) {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }
    
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WriteMulti(VL53L1_DEV dev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t tx_buf[256];
    
    if (count > sizeof(tx_buf) - 2) {
        return VL53L1_ERROR_INVALID_PARAMS;
    }
    
    tx_buf[0] = index >> 8;
    tx_buf[1] = index&0xFF;
    memcpy(&tx_buf[2], pdata, count);
    
    chThdSleepMicroseconds(1300);
    if (i2cMasterTransmitTimeout(dev->bus, dev->i2c_address, tx_buf, count+2, NULL, 0, TIME_INFINITE) != I2C_NO_ERROR) {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }
    
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_RdWord(VL53L1_DEV dev, uint16_t index, uint16_t *data) {
    uint8_t tx_buf[2] = {index >> 8, index&0xFF};
    uint8_t rx_buf[2];

    chThdSleepMicroseconds(1300);
    if (i2cMasterTransmitTimeout(dev->bus, dev->i2c_address, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf), TIME_INFINITE) != I2C_NO_ERROR) {
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }
    
    *data = ((uint16_t)rx_buf[0]<<8) + (uint16_t)rx_buf[1];
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, int32_t wait_ms){
    (void)pdev;
    chThdSleep(LL_MS2ST(wait_ms));
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, int32_t wait_us){
    (void)pdev;
    chThdSleep(LL_US2ST(wait_us));
    return VL53L1_ERROR_NONE;
}

VL53L1_Error VL53L1_WaitValueMaskEx(
    VL53L1_Dev_t *pdev,
    uint32_t      timeout_ms,
    uint16_t      index,
    uint8_t       value,
    uint8_t       mask,
    uint32_t      poll_delay_ms)
{

    /*
     * Platform implementation of WaitValueMaskEx V2WReg script command
     *
     * WaitValueMaskEx(
     *          duration_ms,
     *          index,
     *          value,
     *          mask,
     *          poll_delay_ms);
     */

    VL53L1_Error status         = VL53L1_ERROR_NONE;
    uint32_t     start_time_ms = 0;
    uint32_t     current_time_ms = 0;
    uint32_t     polling_time_ms = 0;
    uint8_t      byte_value      = 0;
    uint8_t      found           = 0;
#ifdef VL53L1_LOG_ENABLE
    uint8_t      trace_functions = VL53L1_TRACE_FUNCTION_NONE;
#endif

    char   register_name[VL53L1_MAX_STRING_LENGTH];

    /* look up register name */
#ifdef PAL_EXTENDED
    VL53L1_get_register_name(
            index,
            register_name);
#else
    VL53L1_COPYSTRING(register_name, "");
#endif

    /* Output to I2C logger for FMT/DFT  */

    /*trace_i2c("WaitValueMaskEx(%5d, 0x%04X, 0x%02X, 0x%02X, %5d);\n",
                 timeout_ms, index, value, mask, poll_delay_ms); */
    trace_i2c("WaitValueMaskEx(%5d, %s, 0x%02X, 0x%02X, %5d);\n",
                 timeout_ms, register_name, value, mask, poll_delay_ms);

    /* calculate time limit in absolute time */

     VL53L1_GetTickCount(&start_time_ms);

    /* remember current trace functions and temporarily disable
     * function logging
     */

#ifdef VL53L1_LOG_ENABLE
    trace_functions = VL53L1_get_trace_functions();
    VL53L1_set_trace_functions(VL53L1_TRACE_FUNCTION_NONE);
#endif

    /* wait until value is found, timeout reached on error occurred */

    while ((status == VL53L1_ERROR_NONE) &&
           (polling_time_ms < timeout_ms) &&
           (found == 0)) {

        if (status == VL53L1_ERROR_NONE)
            status = VL53L1_RdByte(
                            pdev,
                            index,
                            &byte_value);

        if ((byte_value & mask) == value)
            found = 1;

        if (status == VL53L1_ERROR_NONE  &&
            found == 0 &&
            poll_delay_ms > 0)
            status = VL53L1_WaitMs(
                    pdev,
                    poll_delay_ms);

        /* Update polling time (Compare difference rather than absolute to
        negate 32bit wrap around issue) */
        VL53L1_GetTickCount(&current_time_ms);
        polling_time_ms = current_time_ms - start_time_ms;

    }

#ifdef VL53L1_LOG_ENABLE
    /* Restore function logging */
    VL53L1_set_trace_functions(trace_functions);
#endif

    if (found == 0 && status == VL53L1_ERROR_NONE)
        status = VL53L1_ERROR_TIME_OUT;

    return status;
}

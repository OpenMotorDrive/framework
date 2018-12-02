#include <hal.h>
#include <modules/driver_ms5611/driver_ms5611.h>
#include <modules/worker_thread/worker_thread.h>
#include <modules/uavcan_debug/uavcan_debug.h>
#include <uavcan.equipment.air_data.StaticPressure.h>
#include <uavcan.equipment.air_data.StaticTemperature.h>
#include <modules/timing/timing.h>

#define WT hpwork_thread
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct ms5611_instance_s ms5611;
static struct worker_thread_timer_task_s ms5611_task;
static void ms5611_task_func(struct worker_thread_timer_task_s* task);
static struct uavcan_equipment_air_data_StaticTemperature_s temp;
static struct uavcan_equipment_air_data_StaticPressure_s press;

bool ms5611_initialised;

RUN_AFTER(INIT_END) {
    for (uint8_t i = 0; i < 5; i++) {
        if (ms5611_init(&ms5611, 3, BOARD_PAL_LINE_SPI3_MS5611_CS)) {
            ms5611_initialised = true;
            ms5611_measure_temperature(&ms5611);
        }
        usleep(10000);
    }
    worker_thread_add_timer_task(&WT, &ms5611_task, ms5611_task_func, NULL, MS2ST(10), true);
}

static void ms5611_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    static uint8_t _state = 0;
    static uint8_t accum_count;
    if (!ms5611_initialised) {
        if (ms5611_init(&ms5611, 3, BOARD_PAL_LINE_SPI3_MS5611_CS)) {
            ms5611_initialised = true;
            ms5611_measure_temperature(&ms5611);
        }
    } else {
        if (_state == 0) {
            ms5611_accum_temperature(&ms5611);
            ms5611_measure_pressure(&ms5611);
            _state = 1;
        } else if (_state == 1) {
            ms5611_accum_pressure(&ms5611);
            ms5611_measure_temperature(&ms5611);
            if (accum_count >= 2) {
                press.static_pressure = (float)ms5611_read_pressure(&ms5611);
                temp.static_temperature = ((float)ms5611_read_temperature(&ms5611))/100.0f;
                uavcan_broadcast(0, &uavcan_equipment_air_data_StaticPressure_descriptor, CANARD_TRANSFER_PRIORITY_HIGH, &press);
                uavcan_broadcast(0, &uavcan_equipment_air_data_StaticTemperature_descriptor, CANARD_TRANSFER_PRIORITY_HIGH, &temp);
                accum_count = 0;
            } else {
                accum_count++;
            }
            _state = 0;
        }
    }
}

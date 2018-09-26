#include <hal.h>
#include <modules/driver_ak09916/driver_ak09916.h>
#include <modules/worker_thread/worker_thread.h>
#include <modules/uavcan_debug/uavcan_debug.h>
#include <modules/timing/timing.h>

#define WT hpwork_thread
WORKER_THREAD_DECLARE_EXTERN(WT)

#define AK09916_I2C_ADDR 0x0C


static struct ak09916_instance_s ak09916;
static struct icm20x48_instance_s icm20x48;
static struct worker_thread_timer_task_s ak09916_test_task;
static void ak09916_test_task_func(struct worker_thread_timer_task_s* task);
bool ak09916_initialised;
RUN_AFTER(INIT_END) {
    for (uint8_t i = 0; i < 5; i++) {
        if (icm20x48_init(&icm20x48, 3, BOARD_PAL_LINE_SPI3_ICM_CS, ICM20x48_IMU_TYPE_ICM20948)) {
            if (ak09916_init(&ak09916, &icm20x48)) {
                ak09916_initialised = true;
                break;
            }
        }
        usleep(10000);
    }
    worker_thread_add_timer_task(&WT, &ak09916_test_task, ak09916_test_task_func, NULL, MS2ST(1), true);
}

static void ak09916_test_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    if (!ak09916_initialised) {
        if (icm20x48_init(&icm20x48, 3, BOARD_PAL_LINE_SPI3_ICM_CS, ICM20x48_IMU_TYPE_ICM20948)) {
            if (ak09916_init(&ak09916, &icm20x48)) {
                ak09916_initialised = true;
            }
        }
        usleep(10000);
    } else if (ak09916_update(&ak09916)) {
        uavcan_send_debug_keyvalue("magX", ak09916.meas.x);
        uavcan_send_debug_keyvalue("magY", ak09916.meas.y);
        uavcan_send_debug_keyvalue("magZ", ak09916.meas.z);
    }
}

void i2c_serve_interrupt(uint32_t isr) {
    static uint8_t i2c2_transfer_byte_idx;
    static uint8_t i2c2_transfer_address;
    static uint8_t i2c2_transfer_direction;
    if (isr & (1<<3)) { // ADDR
        i2c2_transfer_address = (isr >> 17) & 0x7FU; // ADDCODE
        i2c2_transfer_direction = (isr >> 16) & 1; // direction
        i2c2_transfer_byte_idx = 0;
        if (i2c2_transfer_direction) {
            I2C2->ISR |= (1<<0); // TXE
        }
        I2C2->ICR |= (1<<3); // ADDRCF
    }

    if (isr & I2C_ISR_RXNE) {
        uint8_t recv_byte = I2C2->RXDR & 0xff;; // reading clears our interrupt flag
        switch(i2c2_transfer_address) {
            case AK09916_I2C_ADDR:
                ak09916_recv_byte(i2c2_transfer_byte_idx, recv_byte);
                break;
        }
        i2c2_transfer_byte_idx++;
    }

    if (isr & I2C_ISR_TXIS) {
        uint8_t send_byte = 0;
        switch(i2c2_transfer_address) {
            case AK09916_I2C_ADDR:
                send_byte = ak09916_send_byte();
                break;
        }
    	I2C2->TXDR = send_byte;

        i2c2_transfer_byte_idx++;
    }
}

OSAL_IRQ_HANDLER(STM32_I2C2_EVENT_HANDLER) {
    uint32_t isr = I2C2->ISR;

    OSAL_IRQ_PROLOGUE();

    i2c_serve_interrupt(isr);

    OSAL_IRQ_EPILOGUE();
}

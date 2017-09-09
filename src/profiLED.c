#include "ch.h"
#include "hal.h"
#include <string.h>
#include <common/profiLED.h>
#include <common/helpers.h>

#define MAX_NUM_PROFILEDS 64
#define PROFILED_OUTPUT_BUFFER_SIZE PROFILED_GEN_BUF_SIZE(MAX_NUM_PROFILEDS)
#define PROFILED_WORKER_THREAD_STACK_SIZE (180+PROFILED_OUTPUT_BUFFER_SIZE)
#define PROFILED_WORKER_MAILBOX_DEPTH 2

static void profiLED_assert_chip_select(struct profiLED_instance_s* instance);
static void profiLED_deassert_chip_select(struct profiLED_instance_s* instance);

static const SPIConfig ledSPIConfig = {
    NULL,
    0,
    0,
    0,
    SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};

MAILBOX_DECL(worker_mailbox, worker_mailbox_buf, PROFILED_WORKER_MAILBOX_DEPTH);
static struct profiLED_instance_s* worker_mailbox_buf[PROFILED_WORKER_MAILBOX_DEPTH];


static thread_t* worker_thread;

static THD_WORKING_AREA(waProfiLEDThread, 180+PROFILED_OUTPUT_BUFFER_SIZE);
static THD_FUNCTION(ProfiLEDThread, arg);

void profiLED_init(struct profiLED_instance_s* instance, SPIDriver* spidriver, uint32_t select_line, bool select_active_high, uint32_t num_leds) {
    if (!instance) {
        return;
    }

    if (!worker_thread) {
        worker_thread = chThdCreateFromHeap(NULL, PROFILED_WORKER_THREAD_STACK_SIZE, "LED", LOWPRIO, ProfiLEDThread, NULL);
        if (!worker_thread) goto fail;
        chThdRelease(worker_thread);
    }

    instance->select_line = select_line;
    instance->select_active_high = select_active_high;

    profiLED_deassert_chip_select(instance);

    instance->spidriver = spidriver;
    instance->num_leds = num_leds;

    size_t colors_size = sizeof(struct profiLED_gen_color_s) * instance->num_leds;
    instance->colors = chHeapAlloc(NULL, colors_size);
    if (!instance->colors) goto fail;
    memset(instance->colors, 0, colors_size);

fail:
    chHeapFree(instance->colors);
}

void profiLED_update(struct profiLED_instance_s* instance) {
    // signal the worker thread to update this LED instance
    chMBPost(&worker_mailbox, instance, TIME_IMMEDIATE);
}

void profiLED_set_color_rgb(struct profiLED_instance_s* instance, uint32_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!instance || !instance->colors) {
        return;
    }

    profiLED_gen_make_brg_color_rgb(r, g, b, &instance->colors[idx]);
}

void profiLED_set_color_hex(struct profiLED_instance_s* instance, uint32_t idx, uint32_t color) {
    profiLED_set_color_rgb(instance, idx, (uint8_t)(color>>16), (uint8_t)(color>>8), (uint8_t)color);
}

static THD_FUNCTION(ProfiLEDThread, arg) {
    (void)arg;

    uint8_t txbuf[PROFILED_OUTPUT_BUFFER_SIZE];

    while(true) {
        struct profiLED_instance_s* instance;
        chMBFetch(&worker_mailbox, &instance, TIME_INFINITE);

        if (!instance) continue;

        uint32_t buf_len = profiLED_gen_write_buf(instance->num_leds, instance->colors, txbuf, PROFILED_OUTPUT_BUFFER_SIZE);
        if (buf_len == 0) continue; // NOTE: due to a bug in chibios, calling spiSend with a length of 0 blocks forever

        spiAcquireBus(instance->spidriver);
        spiStart(instance->spidriver, &ledSPIConfig);
        profiLED_assert_chip_select(instance);

        spiSend(instance->spidriver, buf_len, txbuf);

        profiLED_deassert_chip_select(instance);
        spiReleaseBus(instance->spidriver);
    }
}

static void profiLED_assert_chip_select(struct profiLED_instance_s* instance) {
    if (instance->select_active_high) {
        palSetLine(instance->select_line);
    } else {
        palClearLine(instance->select_line);
    }
}

static void profiLED_deassert_chip_select(struct profiLED_instance_s* instance) {
    if (instance->select_active_high) {
        palClearLine(instance->select_line);
    } else {
        palSetLine(instance->select_line);
    }
}

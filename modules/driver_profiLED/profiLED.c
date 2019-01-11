#include <ch.h>
#include <string.h>
#include "profiLED.h"
#include <common/helpers.h>
#include <app_config.h>

#ifndef MAX_NUM_PROFILEDS
#define MAX_NUM_PROFILEDS 64
#endif

#define PROFILED_OUTPUT_BUFFER_SIZE PROFILED_GEN_BUF_SIZE(MAX_NUM_PROFILEDS)
#define PROFILED_WORKER_THREAD_STACK_SIZE 256
#define PROFILED_WORKER_MAILBOX_DEPTH 2

#ifndef PROFILED_BLOCKING
static struct profiLED_instance_s* worker_mailbox_buf[PROFILED_WORKER_MAILBOX_DEPTH];
MAILBOX_DECL(worker_mailbox, worker_mailbox_buf, PROFILED_WORKER_MAILBOX_DEPTH);
static thread_t* worker_thread;

static THD_WORKING_AREA(waProfiLEDThread, PROFILED_WORKER_THREAD_STACK_SIZE);
static THD_FUNCTION(ProfiLEDThread, arg);
#endif

static void _profiled_update(struct profiLED_instance_s* instance);

void profiLED_init(struct profiLED_instance_s* instance, uint8_t spi_bus_idx, uint32_t spi_sel_line, bool sel_active_high, uint32_t num_leds) {
    if (!instance) {
        return;
    }

#ifndef PROFILED_BLOCKING
    if (!worker_thread) {
        worker_thread = chThdCreateStatic(waProfiLEDThread, sizeof(waProfiLEDThread), LOWPRIO, ProfiLEDThread, NULL);
        if (!worker_thread) goto fail;
    }
#endif

    size_t colors_size = sizeof(struct profiLED_gen_color_s) * num_leds;

    instance->colors = chCoreAlloc(colors_size);
    if (!instance->colors) goto fail;
    memset(instance->colors, 0, colors_size);

    if (!spi_device_init(&(instance->dev), spi_bus_idx, spi_sel_line, 30000000, 8, (sel_active_high?SPI_DEVICE_FLAG_SELPOL:0))) {
        goto fail;
    }

    instance->num_leds = num_leds;

    profiLED_update(instance);
    return;

fail:
    instance->colors = NULL;
}

void profiLED_update(struct profiLED_instance_s* instance) {
#ifdef PROFILED_BLOCKING
    _profiled_update(instance);
#else
    // signal the worker thread to update this LED instance
    chMBPost(&worker_mailbox, (msg_t)instance, TIME_IMMEDIATE);
#endif
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

static void _profiled_update(struct profiLED_instance_s* instance) {
    if (!instance) {
        return;
    }

    uint8_t txbuf[PROFILED_OUTPUT_BUFFER_SIZE];
    uint32_t buf_len = profiLED_gen_write_buf(instance->num_leds, instance->colors, txbuf, sizeof(txbuf));
    if (buf_len == 0) {
        return;
    }

    spi_device_send(&(instance->dev), buf_len, txbuf);
}

#ifndef PROFILED_BLOCKING
static THD_FUNCTION(ProfiLEDThread, arg) {
    (void)arg;

    while(true) {
        msg_t instance;
        chMBFetch(&worker_mailbox, &instance, TIME_INFINITE);
        _profiled_update((struct profiLED_instance_s*)instance);
    }
}
#endif

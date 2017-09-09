#pragma once

#include <ch.h>
#include <common/profiLED_gen.h>

struct profiLED_instance_s {
    SPIDriver* spidriver;
    uint32_t select_line;
    bool select_active_high;
    uint32_t num_leds;
    struct profiLED_gen_color_s* colors;
};

void profiLED_init(struct profiLED_instance_s* instance, SPIDriver* spidriver, uint32_t select_line, bool select_active_high, uint32_t num_leds);
void profiLED_update(struct profiLED_instance_s* instance);
void profiLED_set_color_rgb(struct profiLED_instance_s* instance, uint32_t idx, uint8_t r, uint8_t g, uint8_t b);
void profiLED_set_color_hex(struct profiLED_instance_s* instance, uint32_t idx, uint32_t color);

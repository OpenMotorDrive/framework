#include "profiLED_gen.h"
#include <stdint.h>

typedef void (*profiLED_gen_write_byte_func_internal_t)(uint32_t index, uint8_t byte, void* context);

void profiLED_gen_make_brg_color_rgb(uint8_t r, uint8_t g, uint8_t b, struct profiLED_gen_color_s* ret) {
    ret->bytes[0] = b;
    ret->bytes[1] = r;
    ret->bytes[2] = g;
}

void profiLED_gen_make_brg_color_hex(uint32_t color, struct profiLED_gen_color_s* ret) {
    profiLED_gen_make_brg_color_rgb((uint8_t)(color>>16), (uint8_t)(color>>8), (uint8_t)color, ret);
}

static uint8_t _profiLED_gen_get_output_bit(uint32_t num_leds, struct profiLED_gen_color_s* profiLED_gen_colors, uint32_t out_bit_idx) {
    const uint32_t min_bits = num_leds*25+50;
    const uint8_t num_leading_zeros = 8-min_bits%8 + 50;

    if (out_bit_idx < num_leading_zeros) {
        return 0;
    } else if ((out_bit_idx-num_leading_zeros) % 25 == 0) {
        return 1;
    } else {
        uint8_t* profiLED_gen_color_byte_array = (uint8_t*)profiLED_gen_colors;
        uint32_t in_bit_idx = out_bit_idx - num_leading_zeros - (out_bit_idx - num_leading_zeros)/25;
        return (profiLED_gen_color_byte_array[in_bit_idx/8] >> (8-in_bit_idx%8)) & 1;
    }
}

static uint32_t _profiLED_gen_write(uint32_t num_leds, struct profiLED_gen_color_s* profiLED_gen_colors, profiLED_gen_write_byte_func_internal_t write_byte, void* context) {
    const uint32_t min_bits = num_leds*25+50;
    const uint32_t output_stream_length = (min_bits+7)/8;

    uint8_t* profiLED_gen_color_byte_array = (uint8_t*)profiLED_gen_colors;

    for (uint32_t output_idx = 0; output_idx < output_stream_length; output_idx++) {
        uint8_t out_byte = 0;
        for (uint8_t i=0; i<8; i++) {
            out_byte |= _profiLED_gen_get_output_bit(num_leds, profiLED_gen_colors, output_idx*8+i)<<(7-i);
        }
        write_byte(output_idx, out_byte, context);
    }

    return output_stream_length;
}

static void _profiLED_gen_call_write_byte_cb(uint32_t index, uint8_t byte, void* context) {
    (void)index;
    ((profiLED_gen_write_byte_func_t)context)(byte);
}

uint32_t profiLED_gen_write(uint32_t num_leds, struct profiLED_gen_color_s* profiLED_gen_colors, profiLED_gen_write_byte_func_t write_byte) {
    return _profiLED_gen_write(num_leds, profiLED_gen_colors, _profiLED_gen_call_write_byte_cb, (void*)write_byte);
}

static void _profiLED_gen_write_buf_cb(uint32_t index, uint8_t byte, void* context) {
    uint8_t* buf = context;
    buf[index] = byte;
}

uint32_t profiLED_gen_write_buf(uint32_t num_leds, struct profiLED_gen_color_s* profiLED_gen_colors, uint8_t* buf, uint32_t buf_size) {
    if (buf_size < PROFILED_GEN_BUF_SIZE(num_leds)) {
        return 0;
    }

    return _profiLED_gen_write(num_leds, profiLED_gen_colors, _profiLED_gen_write_buf_cb, (void*)buf);
}

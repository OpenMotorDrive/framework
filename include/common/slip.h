#pragma once

#include <stdint.h>

#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

// appends a SLIP-encoded byte to out_buf and increments *out_buf_len
// returns 0 on failure, 1 on success
uint8_t slip_encode_and_append(uint8_t byte, uint8_t* out_buf_len, uint8_t *out_buf, uint8_t out_buf_size);

// decodes the first SLIP frame in in_buf, writes it to out_buf
// assumes sizeof(out_buf) >= (in_len - 1)
// returns out_buf length or 0 on failure to find a valid frame
uint8_t slip_decode(uint8_t in_len, uint8_t *in_buf, uint8_t *out_buf);

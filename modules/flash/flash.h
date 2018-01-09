#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct flash_write_buf_s {
    size_t len;
    const void* data;
};

bool flash_erase_page(void* page_addr);
bool flash_write(void* address, uint8_t num_bufs, struct flash_write_buf_s* bufs);
int16_t flash_get_page_num(void *address);
void* flash_get_page_addr(uint32_t page);
uint32_t flash_get_page_ofs(uint32_t page);
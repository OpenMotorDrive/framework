#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <libopencm3/stm32/flash.h>

bool flash_program_half_word(uint16_t* addr, const uint16_t* src);
bool flash_erase_page(void* addr);

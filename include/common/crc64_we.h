#pragma once

#include <stdint.h>

uint64_t crc64_we(const uint8_t *buf, uint32_t len, uint64_t crc);

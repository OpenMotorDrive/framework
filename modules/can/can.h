#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <hal.h>

void can_init(uint8_t can_idx, uint32_t baud, bool silent);
CANDriver* can_get_device(uint8_t can_idx);

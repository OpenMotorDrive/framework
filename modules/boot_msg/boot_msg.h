#pragma once

#include <stdbool.h>
#include <common/shared_boot_msg.h>

extern const enum shared_msg_t boot_msg_id;
extern const union shared_msg_payload_u boot_msg;
bool get_boot_msg_valid(void);
void boot_msg_fill_shared_canbus_info(struct shared_canbus_info_s* ret);

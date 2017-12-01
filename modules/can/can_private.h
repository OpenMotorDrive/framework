#pragma once
#include "can.h"

void can_tx_frame_completed(struct can_instance_s instance, struct can_tx_frame_s* frame, bool success, systime_t completion_systime);

#include "can_helpers.h"

bool can_tx_frame_expired_X(struct can_tx_frame_s* frame) {
    return chVTGetSystemTimeX() - frame->creation_systime > frame->tx_timeout;
}

systime_t can_tx_frame_time_until_expire_X(struct can_tx_frame_s* frame, systime_t t_now) {
    if (frame->tx_timeout == TIME_INFINITE) {
        return TIME_INFINITE;
    }

    systime_t time_elapsed = t_now - frame->creation_systime;
    if (time_elapsed > frame->tx_timeout) {
        return TIME_IMMEDIATE;
    }

    return frame->tx_timeout - time_elapsed;
}

can_frame_priority_t can_get_frame_priority_X(const struct can_frame_s* frame) {
    can_frame_priority_t ret = 0;

    if (frame->IDE) {
        ret |= ((frame->EID >> 18) & 0x7ff) << 21; // Identifier A
        ret |= 1<<20; // SRR
        ret |= 1<<19; // EID
        ret |= (frame->EID & 0x3ffff) << 1; // Identifier B
        ret |= frame->RTR; // RTR
    } else {
        ret |= frame->SID << 21; // Identifier
        ret |= frame->RTR << 20; // RTR
    }

    return ~ret;
}

can_frame_priority_t can_get_tx_frame_priority_X(const struct can_tx_frame_s* frame) {
    return can_get_frame_priority_X(&frame->content);
}

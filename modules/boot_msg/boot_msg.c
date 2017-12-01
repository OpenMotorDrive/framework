#include <common/ctor.h>
#include <common/shared_boot_msg.h>
#include <string.h>

#ifdef MODULE_UAVCAN_ENABLED
#include <modules/uavcan/uavcan.h>
#include <modules/can/can.h>
#endif

enum shared_msg_t boot_msg_id;
union shared_msg_payload_u boot_msg;
static bool boot_msg_valid;

RUN_ON(BOOT_MSG_RETRIEVAL) {
    boot_msg_valid = shared_msg_check_and_retreive(&boot_msg_id, &boot_msg);
    shared_msg_clear();
}

bool get_boot_msg_valid(void) {
    return boot_msg_valid;
}

void boot_msg_fill_shared_canbus_info(struct shared_canbus_info_s* ret) {
    memset(ret,0,sizeof(struct shared_canbus_info_s));

#ifdef MODULE_UAVCAN_ENABLED
    ret->local_node_id = uavcan_get_node_id(0);

    if (can_get_baudrate_confirmed(can_get_instance(0))) {
        ret->baudrate = can_get_baudrate(can_get_instance(0));
    }
#endif
}

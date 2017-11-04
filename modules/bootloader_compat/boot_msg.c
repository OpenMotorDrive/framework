#include <common/ctor.h>
#include <common/shared_boot_msg.h>

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

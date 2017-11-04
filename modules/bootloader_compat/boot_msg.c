#include <common/ctor.h>
#include <common/shared_boot_msg.h>

enum shared_msg_t shared_msgid;
union shared_msg_payload_u shared_msg;
bool shared_msg_valid;

RUN_BEFORE(CH_SYS_INIT) {
    shared_msg_valid = shared_msg_check_and_retreive(&shared_msgid, &shared_msg);
    shared_msg_clear();
}

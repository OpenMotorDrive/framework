#include <uavcan/uavcan.h>
#include <pubsub/pubsub.h>
#include <common/ctor.h>
#include <string.h>
#include <lpwork_thread/lpwork_thread.h>

#include <uavcan.protocol.param.GetSet.h>
#include <uavcan.protocol.debug.LogMessage.h>

static void param_getset_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;
    const struct uavcan_deserialized_message_s* wrapper = buf;
    const struct uavcan_protocol_param_GetSet_req_s* msg = (const struct uavcan_protocol_param_GetSet_req_s*)wrapper->msg;

    struct uavcan_protocol_debug_LogMessage_s log_message;
    log_message.level.value = UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_DEBUG;
    log_message.source_len = 0;
    strcpy((char*)log_message.text, "got param getset");
    log_message.text_len = strlen((char*)log_message.text);
    uavcan_broadcast(0, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &log_message);
}

static struct worker_thread_listener_task_s task;
static struct pubsub_listener_s getset_listener;
RUN_AFTER(OMD_UAVCAN_INIT) {
    struct pubsub_topic_s* getset_topic = uavcan_get_message_topic(0, &uavcan_protocol_param_GetSet_req_descriptor);

    pubsub_init_and_register_listener(getset_topic, &getset_listener);
    pubsub_listener_set_handler_cb(&getset_listener, param_getset_handler, NULL);

    worker_thread_add_listener_task(&lpwork_thread, &task, &getset_listener);
}

#include <uavcan/uavcan.h>
#include <pubsub/pubsub.h>
#include <common/ctor.h>
#include <string.h>

#include <uavcan.protocol.param.GetSet.h>
#include <uavcan.protocol.debug.LogMessage.h>

#define PARAM_INTERFACE_THREAD_STACK_SIZE 512
static THD_WORKING_AREA(param_interface_thread_wa, PARAM_INTERFACE_THREAD_STACK_SIZE);
static THD_FUNCTION(param_interface_thread_func, arg);

RUN_AFTER(OMD_UAVCAN_INIT) {
    chThdCreateStatic(param_interface_thread_wa, sizeof(param_interface_thread_wa), LOWPRIO, param_interface_thread_func, NULL);
}

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

static THD_FUNCTION(param_interface_thread_func, arg) {
    (void)arg;

    struct pubsub_topic_s* getset_topic = uavcan_get_message_topic(0, &uavcan_protocol_param_GetSet_req_descriptor);
    struct pubsub_listener_s getset_listener;
    pubsub_init_and_register_listener(getset_topic, &getset_listener);
    pubsub_listener_set_handler_cb(&getset_listener, param_getset_handler, NULL);

    pubsub_listener_handle_until_timeout(&getset_listener, TIME_INFINITE);
}

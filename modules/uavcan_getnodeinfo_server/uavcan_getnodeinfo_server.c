#include <uavcan.protocol.GetNodeInfo.h>

static struct pubsub_listener_s getnodeinfo_req_listener;
static struct worker_thread_listener_task_s getnodeinfo_req_listener_task;
static void getnodeinfo_req_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* restart_topic = uavcan_get_message_topic(0, &uavcan_protocol_GetNodeInfo_req_descriptor);
    pubsub_init_and_register_listener(restart_topic, &getnodeinfo_req_listener, getnodeinfo_req_handler, NULL);
    worker_thread_add_listener_task(&lpwork_thread, &getnodeinfo_req_listener_task, &getnodeinfo_req_listener);
}

static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_GetNodeInfo_req_s* msg = (const struct uavcan_protocol_GetNodeInfo_req_s*)msg_wrapper->msg;


}

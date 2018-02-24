#include <modules/uavcan/uavcan.h>
#include <uavcan.protocol.RestartNode.h>
#include <modules/system/system.h>
#include <modules/worker_thread/worker_thread.h>

#ifndef UAVCAN_RESTART_WORKER_THREAD
#error Please define UAVCAN_RESTART_WORKER_THREAD in framework_conf.h.
#endif

#define WT UAVCAN_RESTART_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

#ifdef MODULE_BOOT_MSG_ENABLED
#include <modules/boot_msg/boot_msg.h>
#endif

#ifndef UAVCAN_RESTART_DELAY_MS
#define UAVCAN_RESTART_DELAY_MS 10
#endif

static struct worker_thread_timer_task_s delayed_restart_task;
static struct worker_thread_listener_task_s restart_req_listener_task;
static void restart_req_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* restart_topic = uavcan_get_message_topic(0, &uavcan_protocol_RestartNode_req_descriptor);
    worker_thread_add_listener_task(&WT, &restart_req_listener_task, restart_topic, restart_req_handler, NULL);
}

static void delayed_restart_func(struct worker_thread_timer_task_s* task) {
    (void)task;

#ifdef MODULE_BOOT_MSG_ENABLED
    union shared_msg_payload_u msg;
    boot_msg_fill_shared_canbus_info(&msg.canbus_info);
    shared_msg_finalize_and_write(SHARED_MSG_CANBUS_INFO, &msg);
#endif

    NVIC_SystemReset();
}

static void restart_req_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_RestartNode_req_s* msg = (const struct uavcan_protocol_RestartNode_req_s*)msg_wrapper->msg;

    struct uavcan_protocol_RestartNode_res_s res;

    res.ok = false;

    if (msg->magic_number == UAVCAN_PROTOCOL_RESTARTNODE_REQ_MAGIC_NUMBER && system_get_restart_allowed()) {
        res.ok = true;
        worker_thread_add_timer_task(&WT, &delayed_restart_task, delayed_restart_func, NULL, MS2US(UAVCAN_RESTART_DELAY_MS), false);
    }

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

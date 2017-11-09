#include "uavcan_restart.h"
#include <uavcan/uavcan.h>
#include <uavcan.protocol.RestartNode.h>
#include <lpwork_thread/lpwork_thread.h>

#ifdef MODULE_SYSTEM_EVENT_ENABLED
#include <system_event/system_event.h>
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
#include <boot_msg/boot_msg.h>
#endif

#ifndef UAVCAN_RESTART_DELAY_MS
#define UAVCAN_RESTART_DELAY_MS 10
#endif

static restart_allowed_func_ptr_t restart_allowed_cb;
static struct worker_thread_timer_task_s delayed_restart_task;
static struct pubsub_listener_s restart_req_listener;
static struct worker_thread_listener_task_s restart_req_listener_task;
static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* restart_topic = uavcan_get_message_topic(0, &uavcan_protocol_RestartNode_req_descriptor);
    pubsub_init_and_register_listener(restart_topic, &restart_req_listener, allocation_message_handler, NULL);
    worker_thread_add_listener_task(&lpwork_thread, &restart_req_listener_task, &restart_req_listener);
}

void uavcan_restart_set_restart_allowed_cb(restart_allowed_func_ptr_t cb) {
    restart_allowed_cb = cb;
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

static void allocation_message_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_RestartNode_req_s* msg = (const struct uavcan_protocol_RestartNode_req_s*)msg_wrapper->msg;

    if (msg->magic_number == UAVCAN_PROTOCOL_RESTARTNODE_REQ_MAGIC_NUMBER) {
        struct uavcan_protocol_RestartNode_res_s res;
        res.ok = !restart_allowed_cb || restart_allowed_cb();
        uavcan_respond(msg_wrapper->uavcan_idx, &uavcan_protocol_RestartNode_res_descriptor, msg_wrapper->priority, msg_wrapper->transfer_id, msg_wrapper->source_node_id, &res);

        if (res.ok) {
#ifdef MODULE_SYSTEM_EVENT_ENABLED
            system_event_publish(SYSTEM_EVENT_REBOOT_IMMINENT);
#endif
            worker_thread_add_timer_task(&lpwork_thread, &delayed_restart_task, delayed_restart_func, NULL, MS2ST(UAVCAN_RESTART_DELAY_MS), false);
        }
    }
}

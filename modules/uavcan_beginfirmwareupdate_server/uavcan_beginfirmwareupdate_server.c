#include <modules/uavcan/uavcan.h>
#include <uavcan.protocol.file.BeginFirmwareUpdate.h>
#include <modules/lpwork_thread/lpwork_thread.h>
#include <modules/system/system.h>
#include <modules/boot_msg/boot_msg.h>
#include <string.h>

#ifndef UAVCAN_RESTART_DELAY_MS
#define UAVCAN_RESTART_DELAY_MS 10
#endif

static struct worker_thread_timer_task_s delayed_restart_task;
static struct pubsub_listener_s beginfirmwareupdate_req_listener;
static struct worker_thread_listener_task_s beginfirmwareupdate_req_listener_task;
static void beginfirmwareupdate_req_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* beginfirmwareupdate_req_topic = uavcan_get_message_topic(0, &uavcan_protocol_file_BeginFirmwareUpdate_req_descriptor);
    pubsub_init_and_register_listener(beginfirmwareupdate_req_topic, &beginfirmwareupdate_req_listener, beginfirmwareupdate_req_handler, NULL);
    worker_thread_add_listener_task(&lpwork_thread, &beginfirmwareupdate_req_listener_task, &beginfirmwareupdate_req_listener);
}

static void delayed_restart_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    NVIC_SystemReset();
}

static void beginfirmwareupdate_req_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_file_BeginFirmwareUpdate_req_s* req = (const struct uavcan_protocol_file_BeginFirmwareUpdate_req_s*)msg_wrapper->msg;

    struct uavcan_protocol_file_BeginFirmwareUpdate_res_s res;

    res.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RES_ERROR_INVALID_MODE;
    res.optional_error_message_len = 0;

    if (req->source_node_id > 127 || req->image_file_remote_path.path_len > sizeof(((union shared_msg_payload_u*)0)->firmwareupdate_msg.path)-1) {
        res.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RES_ERROR_UNKNOWN;
    } else if (system_get_restart_allowed()) {
        res.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RES_ERROR_OK;

        // Write boot message
        union shared_msg_payload_u new_boot_msg;

        boot_msg_fill_shared_canbus_info(&new_boot_msg.canbus_info);

        memcpy(new_boot_msg.firmwareupdate_msg.path, (char*)req->image_file_remote_path.path, req->image_file_remote_path.path_len);
        new_boot_msg.firmwareupdate_msg.path[req->image_file_remote_path.path_len] = 0;

        if (req->source_node_id != 0) {
            new_boot_msg.firmwareupdate_msg.source_node_id = req->source_node_id;
        } else {
            new_boot_msg.firmwareupdate_msg.source_node_id = msg_wrapper->source_node_id;
        }

        shared_msg_finalize_and_write(SHARED_MSG_CANBUS_INFO, &new_boot_msg);

        worker_thread_add_timer_task(&lpwork_thread, &delayed_restart_task, delayed_restart_func, NULL, MS2ST(UAVCAN_RESTART_DELAY_MS), false);
    }

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

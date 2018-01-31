#include <modules/uavcan_nodestatus_publisher/uavcan_nodestatus_publisher.h>
#include <modules/worker_thread/worker_thread.h>

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
#include <modules/app_descriptor/app_descriptor.h>
#endif

#ifdef MODULE_BOOT_MSG_ENABLED
#include <modules/boot_msg/boot_msg.h>
#endif

#ifndef UAVCAN_GETNODEINFO_SERVER_WORKER_THREAD
#error Please define UAVCAN_GETNODEINFO_SERVER_WORKER_THREAD in framework_conf.h.
#endif

#define WT UAVCAN_GETNODEINFO_SERVER_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

#include <string.h>

#include <uavcan.protocol.GetNodeInfo.h>

static struct worker_thread_listener_task_s getnodeinfo_req_listener_task;
static void getnodeinfo_req_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* getnodeinfo_req_topic = uavcan_get_message_topic(0, &uavcan_protocol_GetNodeInfo_req_descriptor);
    worker_thread_add_listener_task(&WT, &getnodeinfo_req_listener_task, getnodeinfo_req_topic, getnodeinfo_req_handler, NULL);
}

static void getnodeinfo_req_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;

    struct uavcan_protocol_GetNodeInfo_res_s res;
    memset(&res, 0, sizeof(struct uavcan_protocol_GetNodeInfo_res_s));

    res.status = *uavcan_nodestatus_publisher_get_nodestatus_message();

    board_get_unique_id(res.hardware_version.unique_id, sizeof(res.hardware_version.unique_id));

#if defined(TARGET_BOOTLOADER)
    strncpy((char*)res.name, BOARD_CONFIG_HW_NAME, sizeof(res.name));
    res.name_len = strnlen((char*)res.name, sizeof(res.name));
    res.hardware_version.major = BOARD_CONFIG_HW_MAJOR_VER;
    res.hardware_version.minor = BOARD_CONFIG_HW_MINOR_VER;
#elif defined(MODULE_BOOT_MSG_ENABLED)
    if (get_boot_msg_valid() && boot_msg_id == SHARED_MSG_BOOT_INFO && boot_msg.boot_info_msg.hw_info) {
        strncpy((char*)res.name, boot_msg.boot_info_msg.hw_info->hw_name, sizeof(res.name));
        res.name_len = strnlen((char*)res.name, sizeof(res.name));
        res.hardware_version.major = boot_msg.boot_info_msg.hw_info->hw_major_version;
        res.hardware_version.minor = boot_msg.boot_info_msg.hw_info->hw_minor_version;
    }
#endif

#ifdef MODULE_APP_DESCRIPTOR_ENABLED
    res.software_version.major = shared_app_descriptor.major_version;
    res.software_version.minor = shared_app_descriptor.minor_version;
    res.software_version.optional_field_flags = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT |
                                                UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_IMAGE_CRC;
    res.software_version.vcs_commit = shared_app_descriptor.vcs_commit;
    res.software_version.image_crc = *(volatile uint64_t*)&shared_app_descriptor.image_crc;
#endif

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

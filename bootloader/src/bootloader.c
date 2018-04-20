#include <modules/worker_thread/worker_thread.h>
#include <modules/boot_msg/boot_msg.h>
#include <common/shared_app_descriptor.h>
#include <ch.h>
#include <hal.h>
#include <common/crc64_we.h>
#include <modules/flash/flash.h>
#include <modules/uavcan/uavcan.h>
#include <modules/can/can.h>
#include <modules/timing/timing.h>
#include <modules/system/system.h>
#include <modules/uavcan_nodestatus_publisher/uavcan_nodestatus_publisher.h>
#include <uavcan.protocol.file.BeginFirmwareUpdate.h>
#include <uavcan.protocol.file.Read.h>
#include <uavcan.protocol.RestartNode.h>
#include <uavcan.protocol.GetNodeInfo.h>

#if __GNUC__ != 6 || __GNUC_MINOR__ != 3 || __GNUC_PATCHLEVEL__ != 1
#error Please use arm-none-eabi-gcc 6.3.1.
#endif

#ifndef BOOTLOADER_APP_THREAD
#error Please define BOOTLOADER_APP_THREAD in worker_threads_conf.h.
#endif

#define WT BOOTLOADER_APP_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

struct app_header_s {
    uint32_t stacktop;
    uint32_t entrypoint;
};

// NOTE: _app_app_flash_sec_sec and _app_flash_sec_end symbols shall be defined in the ld script
extern uint8_t _app_flash_sec[], _app_flash_sec_end;

// NOTE: BOARD_CONFIG_HW_INFO_STRUCTURE defined in the board config file
static const struct shared_hw_info_s _hw_info = BOARD_CONFIG_HW_INFO_STRUCTURE;

static struct {
    bool in_progress;
    uint32_t ofs;
    uint32_t app_start_ofs;
    uint8_t uavcan_idx;
    uint8_t retries;
    uint8_t source_node_id;
    int32_t last_erased_page;
    struct worker_thread_timer_task_s read_timeout_task;
    char path[201];
} flash_state;

static struct {
    bool in_progress;
    size_t ofs;
    int32_t last_erased_page;
    struct worker_thread_timer_task_s boot_timer_task;
} bootloader_state;

static struct {
    const struct shared_app_descriptor_s* shared_app_descriptor;
    uint64_t image_crc_computed;
    bool image_crc_correct;
    const struct shared_app_parameters_s* shared_app_parameters;
} app_info;

static struct worker_thread_listener_task_s beginfirmwareupdate_req_listener_task;
static struct worker_thread_listener_task_s file_read_res_task;
static struct worker_thread_listener_task_s restart_req_listener_task;
static struct worker_thread_timer_task_s delayed_restart_task;
static struct worker_thread_listener_task_s getnodeinfo_req_listener_task;

static void file_beginfirmwareupdate_request_handler(size_t msg_size, const void* buf, void* ctx);
static void begin_flash_from_path(uint8_t uavcan_idx, uint8_t source_node_id, const char* path);
static void file_read_response_handler(size_t msg_size, const void* buf, void* ctx);
static void do_resend_read_request(void);
static void do_send_read_request(void);
static uint32_t get_app_sec_size(void);
static void start_boot(struct worker_thread_timer_task_s* task);
static void update_app_info(void);
static void corrupt_app(void);
static void boot_app_if_commanded(void);
static void command_boot_if_app_valid(uint8_t boot_reason);
static void start_boot_timer(systime_t timeout);
static void cancel_boot_timer(void);
static bool check_and_start_boot_timer(void);
static void erase_app_page(uint32_t page_num);
static void do_fail_update(void);
static void on_update_complete(void);
static void bootloader_pre_init(void);
static void bootloader_init(void);
static void restart_req_handler(size_t msg_size, const void* buf, void* ctx);
static void delayed_restart_func(struct worker_thread_timer_task_s* task);
static void read_request_response_timeout(struct worker_thread_timer_task_s* task);
static uint32_t get_app_page_from_ofs(uint32_t ofs);
static uint32_t get_app_address_from_ofs(uint32_t ofs);
static void getnodeinfo_req_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(BOOT_MSG_RETRIEVAL) {
    bootloader_pre_init();
}

RUN_BEFORE(INIT_END) {
    bootloader_init();
}

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* beginfirmwareupdate_req_topic = uavcan_get_message_topic(0, &uavcan_protocol_file_BeginFirmwareUpdate_req_descriptor);
    worker_thread_add_listener_task(&WT, &beginfirmwareupdate_req_listener_task, beginfirmwareupdate_req_topic, file_beginfirmwareupdate_request_handler, NULL);

    struct pubsub_topic_s* file_read_topic = uavcan_get_message_topic(0, &uavcan_protocol_file_Read_res_descriptor);
    worker_thread_add_listener_task(&WT, &file_read_res_task, file_read_topic, file_read_response_handler, NULL);

    struct pubsub_topic_s* restart_topic = uavcan_get_message_topic(0, &uavcan_protocol_RestartNode_req_descriptor);
    worker_thread_add_listener_task(&WT, &restart_req_listener_task, restart_topic, restart_req_handler, NULL);

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

    strncpy((char*)res.name, _hw_info.hw_name, sizeof(res.name));
    res.name_len = strnlen((char*)res.name, sizeof(res.name));
    res.hardware_version.major = _hw_info.hw_major_version;
    res.hardware_version.minor = _hw_info.hw_minor_version;

    if (app_info.shared_app_descriptor && app_info.image_crc_correct) {
        res.software_version.optional_field_flags = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT |
        UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_IMAGE_CRC;

        res.software_version.vcs_commit = app_info.shared_app_descriptor->vcs_commit;
        res.software_version.image_crc = app_info.shared_app_descriptor->image_crc;

        res.software_version.major = app_info.shared_app_descriptor->major_version;
        res.software_version.minor = app_info.shared_app_descriptor->minor_version;
    }

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

static void file_beginfirmwareupdate_request_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;
    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_file_BeginFirmwareUpdate_req_s* req = (const struct uavcan_protocol_file_BeginFirmwareUpdate_req_s*)msg_wrapper->msg;

    struct uavcan_protocol_file_BeginFirmwareUpdate_res_s res;
    if (!flash_state.in_progress) {
        res.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RES_ERROR_OK;
        uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
        char path[sizeof(req->image_file_remote_path)+1] = {};
        memcpy(path, req->image_file_remote_path.path, req->image_file_remote_path.path_len);

        begin_flash_from_path(msg_wrapper->uavcan_idx, req->source_node_id != 0 ? req->source_node_id : msg_wrapper->source_node_id, path);
    } else {
        res.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RES_ERROR_IN_PROGRESS;
        uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
    }
}

static void begin_flash_from_path(uint8_t uavcan_idx, uint8_t source_node_id, const char* path) {
    cancel_boot_timer();
    memset(&flash_state, 0, sizeof(flash_state));
    flash_state.in_progress = true;
    flash_state.ofs = 0; 
    flash_state.source_node_id = source_node_id;
    flash_state.uavcan_idx = uavcan_idx;
    strncpy(flash_state.path, path, 200);
    worker_thread_add_timer_task(&WT, &flash_state.read_timeout_task, read_request_response_timeout, NULL, LL_MS2ST(500), false);
    do_send_read_request();

    corrupt_app();
    flash_state.last_erased_page = -1;
}

static void file_read_response_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;
    if (flash_state.in_progress) {
        const struct uavcan_deserialized_message_s* msg_wrapper = buf;
        const struct uavcan_protocol_file_Read_res_s *res = (const struct uavcan_protocol_file_Read_res_s*)msg_wrapper->msg;

        if (res->error.value != 0 || flash_state.ofs + res->data_len > get_app_sec_size()) {
            do_fail_update();
            return;
        }

        int32_t curr_page = get_app_page_from_ofs(flash_state.ofs + res->data_len);
        if (curr_page > flash_state.last_erased_page) {
            for (int32_t i=flash_state.last_erased_page+1; i<=curr_page; i++) {
                erase_app_page(i);
            }
        }
        struct flash_write_buf_s buf = {res->data_len, (void*)res->data};
        flash_write((void*)get_app_address_from_ofs(flash_state.ofs), 1, &buf);

        if (res->data_len < 256) {
            on_update_complete();
        } else {
            flash_state.ofs += res->data_len;
            do_send_read_request();
        }
    }
}

static void do_resend_read_request(void) {
    struct uavcan_protocol_file_Read_req_s read_req;
    read_req.offset =  flash_state.ofs;
    strncpy(read_req.path.path,flash_state.path,sizeof(read_req.path));
    read_req.path.path_len = strnlen(flash_state.path,sizeof(read_req.path));
    uavcan_request(flash_state.uavcan_idx, &uavcan_protocol_file_Read_req_descriptor, CANARD_TRANSFER_PRIORITY_HIGH, flash_state.source_node_id, &read_req);
    worker_thread_timer_task_reschedule(&WT, &flash_state.read_timeout_task, LL_MS2ST(500));
    flash_state.retries++;
}

static void do_send_read_request(void) {
    do_resend_read_request();
    flash_state.retries = 0;
}

static uint32_t get_app_sec_size(void) {
    return (uint32_t)&_app_flash_sec_end - (uint32_t)&_app_flash_sec[0];
}

static uint32_t get_app_page_from_ofs(uint32_t ofs)
{
    return flash_get_page_num((void*)((uint32_t)&_app_flash_sec[0] + ofs)) - flash_get_page_num((void*)((uint32_t)&_app_flash_sec[0]));
}

static uint32_t get_app_address_from_ofs(uint32_t ofs)
{
    return (uint32_t)&_app_flash_sec[0] + ofs;
}

static void update_app_info(void) {
    memset(&app_info, 0, sizeof(app_info));

    app_info.shared_app_descriptor = shared_find_app_descriptor(_app_flash_sec, get_app_sec_size());

    const struct shared_app_descriptor_s* descriptor = app_info.shared_app_descriptor;


    if (descriptor && descriptor->image_size >= sizeof(struct shared_app_descriptor_s) && descriptor->image_size <= get_app_sec_size()) {
        uint32_t pre_crc_len = ((uint32_t)&descriptor->image_crc) - ((uint32_t)_app_flash_sec);
        uint32_t post_crc_len = descriptor->image_size - pre_crc_len - sizeof(uint64_t);
        uint8_t* pre_crc_origin = _app_flash_sec;
        uint8_t* post_crc_origin = (uint8_t*)((&descriptor->image_crc)+1);
        uint64_t zero64 = 0;

        app_info.image_crc_computed = crc64_we(pre_crc_origin, pre_crc_len, 0);
        app_info.image_crc_computed = crc64_we((uint8_t*)&zero64, sizeof(zero64), app_info.image_crc_computed);
        app_info.image_crc_computed = crc64_we(post_crc_origin, post_crc_len, app_info.image_crc_computed);

        app_info.image_crc_correct = (app_info.image_crc_computed == descriptor->image_crc);
    }
    if (flash_state.in_progress) {
        set_node_health(UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK);
        set_node_mode(UAVCAN_PROTOCOL_NODESTATUS_MODE_SOFTWARE_UPDATE);
    } else {
        set_node_health(app_info.image_crc_correct ? UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK : UAVCAN_PROTOCOL_NODESTATUS_HEALTH_CRITICAL);
        set_node_mode(UAVCAN_PROTOCOL_NODESTATUS_MODE_MAINTENANCE);
    }
    if (app_info.image_crc_correct) {
        app_info.shared_app_parameters = shared_get_parameters(descriptor);
    }
}

static void corrupt_app(void) {
    uint8_t buf[8] = {};
    struct flash_write_buf_s buf_struct = {1, buf};
    flash_write(&_app_flash_sec, 1, &buf_struct);
    update_app_info();
}

static void boot_app_if_commanded(void)
{
    if (!get_boot_msg_valid() || boot_msg_id != SHARED_MSG_BOOT) {
        return;
    }

    union shared_msg_payload_u msg;
    boot_msg_fill_shared_canbus_info(&msg.canbus_info);
    msg.boot_info_msg.boot_reason = boot_msg.boot_msg.boot_reason;
    msg.boot_info_msg.hw_info = &_hw_info;

    shared_msg_finalize_and_write(SHARED_MSG_BOOT_INFO, &msg);

    struct app_header_s* app_header = (struct app_header_s*)_app_flash_sec;

    // offset the vector table
    SCB->VTOR = (uint32_t)&(app_header->stacktop);

    asm volatile(
        "msr msp, %0	\n"
        "bx	%1	\n"
        : : "r"(app_header->stacktop), "r"(app_header->entrypoint) :);
}

static void command_boot_if_app_valid(uint8_t boot_reason) {
    if (!app_info.image_crc_correct) {
        return;
    }

    union shared_msg_payload_u msg;
    msg.boot_msg.boot_reason = boot_reason;

    if (get_boot_msg_valid() && boot_msg.canbus_info.baudrate) {
        msg.boot_msg.canbus_info.baudrate = boot_msg.canbus_info.baudrate;
    }

#ifdef MODULE_CAN_ENABLED
    if (can_get_baudrate_confirmed(0)) {
        msg.boot_msg.canbus_info.baudrate = can_get_baudrate(0);
    } else {
        msg.boot_msg.canbus_info.baudrate = 0;
    }
#endif

    shared_msg_finalize_and_write(SHARED_MSG_BOOT, &msg);

    system_restart();
}

static void start_boot(struct worker_thread_timer_task_s* task)
{
    (void)task;
    command_boot_if_app_valid(SHARED_BOOT_REASON_TIMEOUT);
}

static void start_boot_timer(systime_t timeout) {
    worker_thread_add_timer_task(&WT, &bootloader_state.boot_timer_task, start_boot, NULL, timeout, false);
}

static void cancel_boot_timer(void) {
    worker_thread_remove_timer_task(&WT, &bootloader_state.boot_timer_task);
}

static bool check_and_start_boot_timer(void) {
    if (app_info.shared_app_parameters && app_info.shared_app_parameters->boot_delay_sec != 0) {
        start_boot_timer(S2ST((uint32_t)app_info.shared_app_parameters->boot_delay_sec));
        return true;
    }
    return false;
}

static void erase_app_page(uint32_t page_num) {
    flash_erase_page(flash_get_page_addr(flash_get_page_num(_app_flash_sec) + page_num));
    flash_state.last_erased_page = page_num;
}

static void do_fail_update(void) {
    memset(&flash_state, 0, sizeof(flash_state));
    corrupt_app();
}

static void on_update_complete(void) {
    flash_state.in_progress = false;
    worker_thread_remove_timer_task(&WT, &flash_state.read_timeout_task);
    update_app_info();
    command_boot_if_app_valid(SHARED_BOOT_REASON_FIRMWARE_UPDATE);
}

// TODO: hook this into early_init
// TODO: boot_msg module will have to initialize before this runs
static void bootloader_pre_init(void) {
    boot_app_if_commanded();
}

static void bootloader_init(void) {
    update_app_info();

    if (get_boot_msg_valid() && boot_msg_id == SHARED_MSG_FIRMWAREUPDATE) {
        begin_flash_from_path(0, boot_msg.firmwareupdate_msg.source_node_id, boot_msg.firmwareupdate_msg.path);
    } else {
        check_and_start_boot_timer();
    }
}

static void restart_req_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_RestartNode_req_s* msg = (const struct uavcan_protocol_RestartNode_req_s*)msg_wrapper->msg;

    struct uavcan_protocol_RestartNode_res_s res;

    res.ok = false;

    if ((msg->magic_number == UAVCAN_PROTOCOL_RESTARTNODE_REQ_MAGIC_NUMBER) && system_get_restart_allowed()) {
        res.ok = true;
        worker_thread_add_timer_task(&WT, &delayed_restart_task, delayed_restart_func, NULL, LL_MS2ST(1000), false);
    }

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

static void delayed_restart_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    // try to boot if image is valid
    command_boot_if_app_valid(SHARED_BOOT_REASON_REBOOT_COMMAND);
    // otherwise, just reset
    NVIC_SystemReset();
}

static void read_request_response_timeout(struct worker_thread_timer_task_s* task) {
    (void)task;
    if (flash_state.in_progress) {
        do_resend_read_request();
        if (flash_state.retries > 10) { // retry for 5 seconds
            do_fail_update();
        }
    }
}

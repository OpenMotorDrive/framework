#include <modules/uavcan/uavcan.h>
#include <common/ctor.h>
#include <modules/lpwork_thread/lpwork_thread.h>
#include <uavcan.protocol.debug.LogMessage.h>
#include <modules/faults/faults.h>
#include <string.h>

static struct worker_thread_timer_task_s log_msg_publisher_task;
static struct pubsub_listener_s fault_raised_listener;
static struct worker_thread_listener_task_s fault_raised_listener_task;

static void fault_raised_handler(size_t msg_size, const void* buf, void* ctx);
static void log_msg_publisher_task_func(struct worker_thread_timer_task_s* task);
static void publish_fault(struct fault_s* fault);

RUN_AFTER(UAVCAN_INIT) {
    worker_thread_add_timer_task(&lpwork_thread, &log_msg_publisher_task, log_msg_publisher_task_func, NULL, S2ST(3), true);
    
    pubsub_init_and_register_listener(&fault_raised_topic, &fault_raised_listener, fault_raised_handler, NULL);
    
    worker_thread_add_listener_task(&lpwork_thread, &fault_raised_listener_task, &fault_raised_listener);
}

static void fault_raised_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)ctx;
    (void)msg_size;
    publish_fault(*(struct fault_s**)buf);
}

static void log_msg_publisher_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;
    
    struct fault_s* worst_fault = fault_get_worst_fault();
    
    if (worst_fault) {
        publish_fault(worst_fault);
    }
}

static void publish_fault(struct fault_s* fault) {
    struct uavcan_protocol_debug_LogMessage_s log_msg;
    memset(&log_msg, 0, sizeof(log_msg));

    switch(fault_get_level(fault)) {
        case FAULT_LEVEL_WARNING:
            log_msg.level.value = UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_WARNING;
            break;
        case FAULT_LEVEL_ERROR:
            log_msg.level.value = UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_ERROR;
            break;
        case FAULT_LEVEL_CRITICAL:
            log_msg.level.value = UAVCAN_PROTOCOL_DEBUG_LOGLEVEL_ERROR;
            break;
        default:
            break;
    }

    const char* source = "faults";

    log_msg.source_len = strnlen(source, sizeof(log_msg.source));
    memcpy(log_msg.source, source, log_msg.source_len);

    if (fault->description) {
        log_msg.text_len = strnlen(fault->description, sizeof(log_msg.text));
        memcpy(log_msg.text, fault->description, log_msg.text_len);
    }

    uavcan_broadcast(0, &uavcan_protocol_debug_LogMessage_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &log_msg);
}

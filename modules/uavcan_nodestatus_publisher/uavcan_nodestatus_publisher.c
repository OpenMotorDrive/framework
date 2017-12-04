#include "uavcan_nodestatus_publisher.h"
#include <modules/uavcan/uavcan.h>
#include <common/ctor.h>
#include <modules/worker_thread/worker_thread.h>

#ifndef UAVCAN_NODESTATUS_PUBLISHER_WORKER_THREAD
#error Please define UAVCAN_NODESTATUS_PUBLISHER_WORKER_THREAD in worker_threads_conf.h.
#endif

#define WT UAVCAN_NODESTATUS_PUBLISHER_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

static struct uavcan_protocol_NodeStatus_s node_status;
static struct worker_thread_timer_task_s node_status_publisher_task;

static void node_status_publisher_task_func(struct worker_thread_timer_task_s* task);

// TODO mechanism to change node status

const struct uavcan_protocol_NodeStatus_s* uavcan_nodestatus_publisher_get_nodestatus_message(void) {
    return &node_status;
}

RUN_AFTER(UAVCAN_INIT) {
    node_status.uptime_sec = 0;
    node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    node_status.sub_mode = 0;
    node_status.vendor_specific_status_code = 0;

    worker_thread_add_timer_task(&WT, &node_status_publisher_task, node_status_publisher_task_func, NULL, S2ST(1), true);
}

static void node_status_publisher_task_func(struct worker_thread_timer_task_s* task) {
    (void)task;

    node_status.uptime_sec++;
    uavcan_broadcast(0, &uavcan_protocol_NodeStatus_descriptor, CANARD_TRANSFER_PRIORITY_LOW, &node_status);
}

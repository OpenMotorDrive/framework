#include "can.h"
#include "can_driver.h"
#include "can_tx_queue.h"
#include "can_helpers.h"
#include "string.h"
#include <common/helpers.h>
#include <modules/pubsub/pubsub.h>
#include <modules/lpwork_thread/lpwork_thread.h> // TODO: use high priority worker thread for CAN

#ifndef CAN_TX_QUEUE_LEN
#define CAN_TX_QUEUE_LEN 30
#endif

#define MAX_NUM_TX_MAILBOXES 3

enum can_tx_mailbox_state_t {
    CAN_TX_MAILBOX_EMPTY,
    CAN_TX_MAILBOX_PENDING,
    CAN_TX_MAILBOX_ABORTING
};

struct can_tx_mailbox_s {
    struct can_tx_frame_s* frame;
    enum can_tx_mailbox_state_t state;
};

struct can_instance_s {
    uint8_t idx;

    bool started;
    bool silent;
    bool auto_retransmit;
    uint32_t baudrate;
    bool baudrate_confirmed;

    void* driver_ctx;
    const struct can_driver_iface_s* driver_iface;

    struct can_tx_mailbox_s tx_mailbox[MAX_NUM_TX_MAILBOXES];
    uint8_t num_tx_mailboxes;

    struct can_tx_queue_s tx_queue;

    struct pubsub_topic_s rx_topic;
    struct worker_thread_publisher_task_s rx_publisher_task;

    struct worker_thread_publisher_task_s tx_publisher_task;

    struct worker_thread_timer_task_s expire_timer_task;

    struct can_instance_s* next;
};

MEMORYPOOL_DECL(can_instance_pool, sizeof(struct can_instance_s), chCoreAllocAlignedI);

static struct can_instance_s* can_instance_list_head;

static void can_expire_handler(struct worker_thread_timer_task_s* task);
static void can_reschedule_expire_timer_I(struct can_instance_s* instance);
static void can_try_enqueue_waiting_frame_I(struct can_instance_s* instance);

bool can_iterate_instances(struct can_instance_s** instance_ptr) {
    if (!instance_ptr) {
        return false;
    }

    if (!(*instance_ptr)) {
        *instance_ptr = can_instance_list_head;
    } else {
        *instance_ptr = (*instance_ptr)->next;
    }

    return *instance_ptr != NULL;
}

struct can_instance_s* can_get_instance(uint8_t can_idx) {
    for (struct can_instance_s* instance = can_instance_list_head; instance != NULL; instance = instance->next) {
        if (instance->idx == can_idx) {
            return instance;
        }
    }

    return NULL;
}

struct pubsub_topic_s* can_get_rx_topic(struct can_instance_s* instance) {
    chDbgCheck(instance != NULL);
    if (!instance) {
        return NULL;
    }

    return &instance->rx_topic;
}

uint32_t can_get_baudrate(struct can_instance_s* instance) {
    if (!instance) {
        return 0;
    }

    return instance->baudrate;
}

void can_set_silent_mode(struct can_instance_s* instance, bool silent) {
    if (!instance) {
        return;
    }

    chSysLock();
    if (instance->started && instance->silent != silent) {
        can_start_I(instance, silent, instance->auto_retransmit, instance->baudrate);
    }
    chSysUnlock();
}

void can_set_auto_retransmit_mode(struct can_instance_s* instance, bool auto_retransmit) {
    if (!instance) {
        return;
    }

    chSysLock();
    if (instance->started && instance->auto_retransmit != auto_retransmit) {
        can_start_I(instance, instance->silent, auto_retransmit, instance->baudrate);
    }
    chSysUnlock();
}

void can_set_baudrate(struct can_instance_s* instance, uint32_t baudrate) {
    if (!instance) {
        return;
    }

    chSysLock();
    if (instance->started && instance->baudrate != baudrate) {
        can_start_I(instance, instance->silent, instance->auto_retransmit, baudrate);
    }
    chSysUnlock();
}

bool can_get_baudrate_confirmed(struct can_instance_s* instance) {
    if (!instance) {
        return false;
    }

    return instance->baudrate_confirmed;
}

void can_start_I(struct can_instance_s* instance, bool silent, bool auto_retransmit, uint32_t baudrate) {
    chDbgCheckClassI();
    if (!instance) {
        return;
    }

    if (instance->started) {
        // TODO prevent dropped frames when re-starting CAN driver
        can_stop_I(instance);
    }

    instance->driver_iface->start(instance->driver_ctx, silent, auto_retransmit, baudrate);
    instance->started = true;
    instance->silent = silent;
    instance->auto_retransmit = auto_retransmit;
    if (baudrate != instance->baudrate) {
        instance->baudrate_confirmed = false;
    }
    instance->baudrate = baudrate;
}

void can_start(struct can_instance_s* instance, bool silent, bool auto_retransmit, uint32_t baudrate) {
    chSysLock();
    can_start_I(instance, silent, auto_retransmit, baudrate);
    chSysUnlock();
}

void can_stop_I(struct can_instance_s* instance) {
    if (!instance) {
        return;
    }

    if (instance->started) {
        instance->driver_iface->stop(instance->driver_ctx);
        instance->started = true;
    }
}

void can_stop(struct can_instance_s* instance) {
    chSysLock();
    can_stop_I(instance);
    chSysUnlock();
}

struct can_tx_frame_s* can_allocate_frame_I(struct can_instance_s* instance) {
    chDbgCheckClassI();

    if (!instance) {
        return NULL;
    }

    return can_tx_queue_allocate_I(&instance->tx_queue);
}
void can_stage_frame_I(struct can_instance_s* instance, struct can_tx_frame_s* frame) {
    chDbgCheckClassI();

    if (!instance) {
        return;
    }

    can_tx_queue_stage_push_I(&instance->tx_queue, frame);
}

void can_send_staged_frames_I(struct can_instance_s* instance, systime_t tx_timeout, struct pubsub_topic_s* completion_topic) {
    chDbgCheckClassI();

    if (!instance) {
        return;
    }

    systime_t t_now = chVTGetSystemTimeX();
    struct can_tx_frame_s* frame = NULL;
    while (can_tx_stage_iterate_I(&instance->tx_queue, &frame)) {
        frame->creation_systime = t_now;
        frame->tx_timeout = tx_timeout;
        if (!frame->next) {
            frame->completion_topic = completion_topic;
        }
    }

    can_tx_queue_commit_staged_pushes_I(&instance->tx_queue);
    
    can_try_enqueue_waiting_frame_I(instance);

    can_reschedule_expire_timer_I(instance);
}

void can_free_staged_frames_I(struct can_instance_s* instance) {
    chDbgCheckClassI();

    if (!instance) {
        return;
    }

    can_tx_queue_free_staged_pushes_I(&instance->tx_queue);
}

bool can_send_I(struct can_instance_s* instance, struct can_frame_s* frame, systime_t tx_timeout, struct pubsub_topic_s* completion_topic) {
    chDbgCheckClassI();

    if (!instance) {
        return false;
    }

    struct can_tx_frame_s* tx_frame = can_allocate_frame_I(instance);
    if (!tx_frame) {
        return false;
    }

    tx_frame->content = *frame;

    can_stage_frame_I(instance, tx_frame);
    can_send_staged_frames_I(instance, tx_timeout, completion_topic);
    return true;
}

bool can_send(struct can_instance_s* instance, struct can_frame_s* frame, systime_t tx_timeout, struct pubsub_topic_s* completion_topic) {
    chSysLock();
    bool ret = can_send_I(instance, frame, tx_timeout, completion_topic);
    chSysUnlock();
    return ret;
}

struct can_instance_s* can_driver_register(uint8_t can_idx, void* driver_ctx, const struct can_driver_iface_s* driver_iface, uint8_t num_tx_mailboxes, uint8_t num_rx_mailboxes, uint8_t rx_fifo_depth) {
    if (can_get_instance(can_idx) != NULL) {
        return NULL;
    }

    struct can_instance_s* instance = chPoolAlloc(&can_instance_pool);

    if (instance == NULL) {
        return NULL;
    }

    if (num_tx_mailboxes > MAX_NUM_TX_MAILBOXES) {
        num_tx_mailboxes = MAX_NUM_TX_MAILBOXES;
    }

    instance->idx = can_idx;

    instance->started = false;
    instance->silent = false;
    instance->auto_retransmit = false;
    instance->baudrate = 0;
    instance->baudrate_confirmed = false;

    instance->driver_ctx = driver_ctx;
    instance->driver_iface = driver_iface;

    for (uint8_t i=0; i<MAX_NUM_TX_MAILBOXES; i++) {
        instance->tx_mailbox[i].state = CAN_TX_MAILBOX_EMPTY;
    }
    instance->num_tx_mailboxes = num_tx_mailboxes;

    can_tx_queue_init(&instance->tx_queue, CAN_TX_QUEUE_LEN);

    pubsub_init_topic(&instance->rx_topic, NULL); // TODO specific/configurable topic group
    worker_thread_add_publisher_task(&lpwork_thread, &instance->rx_publisher_task, sizeof(struct can_rx_frame_s), num_rx_mailboxes*rx_fifo_depth);

    worker_thread_add_publisher_task(&lpwork_thread, &instance->tx_publisher_task, sizeof(struct can_transmit_completion_msg_s), num_tx_mailboxes);

    worker_thread_add_timer_task(&lpwork_thread, &instance->expire_timer_task, can_expire_handler, instance, TIME_INFINITE, false);

    LINKED_LIST_APPEND(struct can_instance_s, can_instance_list_head, instance);

    return instance;
}

static void can_try_enqueue_waiting_frame_I(struct can_instance_s* instance) {
    chDbgCheckClassI();
    
    // Enqueue the next frame if it will be the highest priority
    bool have_empty_mailbox = false;
    uint8_t empty_mailbox_idx;
    bool have_pending_mailbox = false;
    can_frame_priority_t highest_prio_pending;
    
    for (uint8_t i=0; i < instance->num_tx_mailboxes; i++) {
        if (instance->tx_mailbox[i].state == CAN_TX_MAILBOX_EMPTY) {
            have_empty_mailbox = true;
            empty_mailbox_idx = i;
        } else if (instance->tx_mailbox[i].state == CAN_TX_MAILBOX_PENDING || instance->tx_mailbox[i].state == CAN_TX_MAILBOX_ABORTING) {
            can_frame_priority_t prio = can_get_tx_frame_priority_X(instance->tx_mailbox[i].frame);
            if (!have_pending_mailbox || prio > highest_prio_pending) {
                highest_prio_pending = prio;
            }
            have_pending_mailbox = true;
        }
    }
    
    if (!have_empty_mailbox) {
        return;
    }
    
    struct can_tx_frame_s* frame = can_tx_queue_peek_I(&instance->tx_queue);
    if (frame && (!have_pending_mailbox || can_get_tx_frame_priority_X(frame) > highest_prio_pending)) {
        can_tx_queue_pop_I(&instance->tx_queue);
        instance->tx_mailbox[empty_mailbox_idx].frame = frame;
        instance->tx_mailbox[empty_mailbox_idx].state = CAN_TX_MAILBOX_PENDING;
        instance->driver_iface->load_tx_mailbox_I(instance->driver_ctx, empty_mailbox_idx, &frame->content);
    }
}

static void can_try_enqueue_waiting_frame(struct can_instance_s* instance) {
    chSysLock();
    can_try_enqueue_waiting_frame_I(instance);
    chSysUnlock();
}

static void can_reschedule_expire_timer_I(struct can_instance_s* instance) {
    chDbgCheckClassI();

    // Find frame that expires soonest in mailboxes and queue, schedule expire handler for that time
    systime_t t_now = chVTGetSystemTimeX();
    systime_t min_ticks_to_expire = TIME_INFINITE;

    for (uint8_t i=0; i < instance->num_tx_mailboxes; i++) {
        if (instance->tx_mailbox[i].state == CAN_TX_MAILBOX_PENDING) {
            systime_t ticks_to_expire = can_tx_frame_time_until_expire_X(instance->tx_mailbox[i].frame, t_now);
            if (ticks_to_expire < min_ticks_to_expire) {
                min_ticks_to_expire = ticks_to_expire;
            }
        }
    }

    struct can_tx_frame_s* frame = NULL;
    while (can_tx_queue_iterate_I(&instance->tx_queue, &frame)) {
        systime_t ticks_to_expire = can_tx_frame_time_until_expire_X(frame, t_now);
        if (ticks_to_expire < min_ticks_to_expire) {
            min_ticks_to_expire = ticks_to_expire;
        }
    }

    worker_thread_timer_task_reschedule_I(&lpwork_thread, &instance->expire_timer_task, min_ticks_to_expire);
}

static void can_reschedule_expire_timer(struct can_instance_s* instance) {
    chSysLock();
    can_reschedule_expire_timer_I(instance);
    chSysUnlock();
}

static void can_tx_frame_completed_I(struct can_instance_s* instance, struct can_tx_frame_s* frame, bool success, systime_t completion_systime) {
    chDbgCheckClassI();

    if (frame->completion_topic) {
        struct can_transmit_completion_msg_s msg = { success, completion_systime };
        worker_thread_publisher_task_publish_I(&instance->tx_publisher_task, frame->completion_topic, sizeof(struct can_transmit_completion_msg_s), pubsub_copy_writer_func, &msg);
    }
    can_tx_queue_free_I(&instance->tx_queue, frame);
}

static void can_tx_frame_completed(struct can_instance_s* instance, struct can_tx_frame_s* frame, bool success, systime_t completion_systime) {
    if (frame->completion_topic) {
        struct can_transmit_completion_msg_s msg = { success, completion_systime };
        pubsub_publish_message(frame->completion_topic, sizeof(struct can_transmit_completion_msg_s), pubsub_copy_writer_func, &msg);
    }
    can_tx_queue_free(&instance->tx_queue, frame);
}

static void can_expire_handler(struct worker_thread_timer_task_s* task) {
    struct can_instance_s* instance = worker_thread_task_get_user_context(task);

    // Abort expired mailboxes
    for (uint8_t i=0; i < instance->num_tx_mailboxes; i++) {
        chSysLock();
        if (instance->tx_mailbox[i].state == CAN_TX_MAILBOX_PENDING && can_tx_frame_expired_X(instance->tx_mailbox[i].frame)) {
            if (instance->driver_iface->abort_tx_mailbox_I(instance->driver_ctx, i)) {
                instance->tx_mailbox[i].state = CAN_TX_MAILBOX_ABORTING;
            }
        }
        chSysUnlock();
    }

    // Abort expired queue items
    struct can_tx_frame_s* frame = NULL;
    while (true) {
        chSysLock();
        if (!can_tx_queue_iterate_I(&instance->tx_queue, &frame) || !can_tx_frame_expired_X(frame)) {
            chSysUnlock();
            break;
        }

        can_tx_queue_remove_I(&instance->tx_queue, frame);
        chSysUnlock();
        can_tx_frame_completed(instance, frame, false, chVTGetSystemTimeX());
    }

    can_try_enqueue_waiting_frame(instance);

    can_reschedule_expire_timer(instance);
}

void can_driver_tx_request_complete_I(struct can_instance_s* instance, uint8_t mb_idx, bool transmit_success, systime_t completion_systime) {
    chDbgCheckClassI();
    chDbgCheck(instance->tx_mailbox[mb_idx].state == CAN_TX_MAILBOX_PENDING || instance->tx_mailbox[mb_idx].state == CAN_TX_MAILBOX_ABORTING);

    can_tx_frame_completed_I(instance, instance->tx_mailbox[mb_idx].frame, transmit_success, completion_systime);
    instance->tx_mailbox[mb_idx].state = CAN_TX_MAILBOX_EMPTY;

    can_try_enqueue_waiting_frame_I(instance);
}

struct can_fill_rx_frame_params_s {
    systime_t rx_systime;
    struct can_frame_s* frame;
};

static void can_fill_rx_frame_I(size_t msg_size, void* msg, void* ctx) {
    (void)msg_size;

    chDbgCheckClassI();

    struct can_fill_rx_frame_params_s* params = ctx;
    struct can_rx_frame_s* frame = msg;

    frame->content = *params->frame;
    frame->rx_systime = params->rx_systime;
}

void can_driver_rx_frame_received_I(struct can_instance_s* instance, uint8_t mb_idx, systime_t rx_systime, struct can_frame_s* frame) {
    (void)mb_idx;

    chDbgCheckClassI();

    struct can_fill_rx_frame_params_s can_fill_rx_frame_params = {rx_systime, frame};
    worker_thread_publisher_task_publish_I(&instance->rx_publisher_task, &instance->rx_topic, sizeof(struct can_rx_frame_s), can_fill_rx_frame_I, &can_fill_rx_frame_params);
    instance->baudrate_confirmed = true;
}

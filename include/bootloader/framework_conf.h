#pragma once

//
// Configure worker threads
//

#define STACK_MEASUREMENT_WORKER_THREAD                 lpwork_thread
#define TIMING_WORKER_THREAD                            lpwork_thread
#define UAVCAN_NODESTATUS_PUBLISHER_WORKER_THREAD       lpwork_thread
#define CAN_AUTOBAUD_WORKER_THREAD                      lpwork_thread
#define UAVCAN_PARAM_INTERFACE_WORKER_THREAD            lpwork_thread
#define UAVCAN_GETNODEINFO_SERVER_WORKER_THREAD         lpwork_thread
#define UAVCAN_RESTART_WORKER_THREAD                    lpwork_thread
#define UAVCAN_BEGINFIRMWAREUPDATE_SERVER_WORKER_THREAD lpwork_thread
#define UAVCAN_ALLOCATEE_WORKER_THREAD                  lpwork_thread
#define BOOTLOADER_APP_THREAD                           lpwork_thread

#define CAN_TRX_WORKER_THREAD                           can_thread
#define CAN_EXPIRE_WORKER_THREAD                        can_thread
#define UAVCAN_RX_WORKER_THREAD                         can_thread

//
// Configure topic groups
//

#define PUBSUB_DEFAULT_TOPIC_GROUP default_topic_group

//
// Misc configs
//

#define REQUIRED_RAM_MARGIN_AFTER_INIT      0
#define PARAM_MAX_NUM_PARAMS                12

//
// Configure debug checks
//

#define CH_DBG_SYSTEM_STATE_CHECK           FALSE
#define CH_DBG_ENABLE_CHECKS                FALSE
#define CH_DBG_ENABLE_ASSERTS               FALSE
#define CH_DBG_ENABLE_STACK_CHECK           FALSE

#ifndef CH_DBG_FILL_THREADS
#define CH_DBG_FILL_THREADS                 FALSE
#endif

#ifndef CH_DBG_STATISTICS
#define CH_DBG_STATISTICS                   FALSE
#endif

#pragma once

#define TIMING_WORKER_THREAD                            lpwork_thread
#define UAVCAN_NODESTATUS_PUBLISHER_WORKER_THREAD       lpwork_thread
#define CAN_AUTOBAUD_WORKER_THREAD                      lpwork_thread
#define UAVCAN_PARAM_INTERFACE_WORKER_THREAD            lpwork_thread
#define UAVCAN_GETNODEINFO_SERVER_WORKER_THREAD         lpwork_thread
#define UAVCAN_RESTART_WORKER_THREAD                    lpwork_thread
#define UAVCAN_BEGINFIRMWAREUPDATE_SERVER_WORKER_THREAD lpwork_thread
#define UAVCAN_ALLOCATEE_WORKER_THREAD                  lpwork_thread

#define CAN_TRX_WORKER_THREAD                           can_thread
#define CAN_EXPIRE_WORKER_THREAD                        can_thread
#define UAVCAN_RX_WORKER_THREAD                         can_thread

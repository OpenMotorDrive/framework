#include <common/uavcan.h>
#include <common/can.h>
#include <common/timing.h>
#include <stdlib.h>
#include <string.h>
#include <canard.h>
#include <board.h>

#define UNUSED(x) ((void)x)
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define BIT_LEN_TO_SIZE(x) ((x+7)/8)

#define UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_ID                      1
#define UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_SIGNATURE               0x0b2a812620a11d40
#define UAVCAN_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_US             600000U
#define UAVCAN_NODE_ID_ALLOCATION_MAX_REQUEST_PERIOD_US             1000000U
#define UAVCAN_NODE_ID_ALLOCATION_MIN_FOLLOWUP_PERIOD_US            0U
#define UAVCAN_NODE_ID_ALLOCATION_MAX_FOLLOWUP_PERIOD_US            400000U
#define UAVCAN_NODE_ID_ALLOCATION_UID_BIT_OFFSET                    8
#define UAVCAN_NODE_ID_ALLOCATION_MAX_LENGTH_OF_UID_IN_REQUEST      6

#define UAVCAN_NODE_STATUS_MESSAGE_SIZE                             7
#define UAVCAN_NODE_STATUS_DATA_TYPE_ID                             341
#define UAVCAN_NODE_STATUS_DATA_TYPE_SIGNATURE                      0x0f0868d0c1a7c6f1

#define UAVCAN_GET_NODE_INFO_RESPONSE_MAX_SIZE                      BIT_LEN_TO_SIZE(3015)
#define UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE                    0xee468a8121c46a9e
#define UAVCAN_GET_NODE_INFO_DATA_TYPE_ID                           1

#define UAVCAN_RESTARTNODE_REQUEST_MAX_SIZE                         BIT_LEN_TO_SIZE(40)
#define UAVCAN_RESTARTNODE_RESPONSE_MAX_SIZE                        BIT_LEN_TO_SIZE(1)
#define UAVCAN_RESTARTNODE_DATA_TYPE_ID                             5
#define UAVCAN_RESTARTNODE_DATA_TYPE_SIGNATURE                      0x569e05394a3017f0

#define UAVCAN_ESC_STATUS_MESSAGE_SIZE                              BIT_LEN_TO_SIZE(110)
#define UAVCAN_ESC_STATUS_DATA_TYPE_ID                              1034
#define UAVCAN_ESC_STATUS_DATA_TYPE_SIGNATURE                       0xa9af28aea2fbb254

#define UAVCAN_DEBUG_KEYVALUE_MESSAGE_MAX_SIZE                      BIT_LEN_TO_SIZE(502)
#define UAVCAN_DEBUG_KEYVALUE_DATA_TYPE_ID                          16370
#define UAVCAN_DEBUG_KEYVALUE_DATA_TYPE_SIGNATURE                   0xe02f25d6e0c98ae0

#define UAVCAN_ESC_RAWCOMMAND_MESSAGE_MAX_SIZE                      BIT_LEN_TO_SIZE(285)
#define UAVCAN_ESC_RAWCOMMAND_DATA_TYPE_ID                          1030
#define UAVCAN_ESC_RAWCOMMAND_DATA_TYPE_SIGNATURE                   0x217f5c87d7ec951d

#define UAVCAN_DEBUG_LOGMESSAGE_MESSAGE_MAX_SIZE                    BIT_LEN_TO_SIZE(983)
#define UAVCAN_DEBUG_LOGMESSAGE_DATA_TYPE_ID                        16383
#define UAVCAN_DEBUG_LOGMESSAGE_DATA_TYPE_SIGNATURE                 0xd654a48e0c049d75

#define UAVCAN_FILE_BEGINFIRMWAREUPDATE_REQUEST_MAX_SIZE            BIT_LEN_TO_SIZE(1616)
#define UAVCAN_FILE_BEGINFIRMWAREUPDATE_RESPONSE_MAX_SIZE           BIT_LEN_TO_SIZE(1031)
#define UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_ID                40
#define UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_SIGNATURE         0xb7d725df72724126

#define UAVCAN_FILE_READ_REQUEST_MAX_SIZE                           BIT_LEN_TO_SIZE(1648)
#define UAVCAN_FILE_READ_RESPONSE_MAX_SIZE                          BIT_LEN_TO_SIZE(2073)
#define UAVCAN_FILE_READ_DATA_TYPE_ID                               48
#define UAVCAN_FILE_READ_DATA_TYPE_SIGNATURE                        0x8dcdca939f33f678

#define UNIQUE_ID_LENGTH_BYTES                                      16

static struct uavcan_node_info_s node_info;

static restart_handler_ptr restart_cb;
static file_beginfirmwareupdate_handler_ptr file_beginfirmwareupdate_cb;
static file_read_response_handler_ptr file_read_response_cb;
static uavcan_ready_handler_ptr uavcan_ready_cb;

static CanardInstance canard;
static uint8_t canard_memory_pool[1024];
static bool canard_initialized;

static uint8_t node_health = UAVCAN_HEALTH_OK;
static uint8_t node_mode   = UAVCAN_MODE_INITIALIZATION;

static struct {
    uint32_t request_timer_begin_us;
    uint32_t request_delay_us;
    uint32_t unique_id_offset;
} allocation_state;

static uint8_t node_unique_id[UNIQUE_ID_LENGTH_BYTES];

static uint32_t started_at_sec;
static uint32_t last_1hz_ms;
static bool called_uavcan_ready_cb;

static float getRandomFloat(void);
static void makeNodeStatusMessage(uint8_t* buffer);
static bool shouldAcceptTransfer(const CanardInstance* ins, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);
static void process1HzTasks(void);
static void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer);
static struct uavcan_transfer_info_s get_transfer_info(const CanardInstance* ins, CanardRxTransfer* transfer);

static void allocation_init(void);
static void allocation_update(void);
static bool allocation_running(void);
static void allocation_timer_expired(void);
static void allocation_start_request_timer(void);
static void allocation_start_followup_timer(void);

void uavcan_init(void)
{
    board_get_unique_id((uint32_t*)&node_unique_id[0]);
    canardInit(&canard, canard_memory_pool, sizeof(canard_memory_pool), onTransferReceived, shouldAcceptTransfer, NULL);
    allocation_init();
    canard_initialized = true;
}

void uavcan_update(void)
{
    if (!canard_initialized) {
        return;
    }

    uint32_t tnow_ms = millis();
    if (tnow_ms-last_1hz_ms >= 1000) {
        process1HzTasks();
        last_1hz_ms = tnow_ms;
    }

    allocation_update();

    if (!allocation_running() && !called_uavcan_ready_cb && uavcan_ready_cb) {
        uavcan_ready_cb();
        called_uavcan_ready_cb = true;
    }

    // receive
    CanardCANFrame rx_frame;
    struct canbus_msg msg;
    const uint64_t timestamp = micros();
    if (canbus_recv_message(&msg)) {
        rx_frame.id = msg.id & CANARD_CAN_EXT_ID_MASK;
        if (msg.ide) rx_frame.id |= CANARD_CAN_FRAME_EFF;
        if (msg.rtr) rx_frame.id |= CANARD_CAN_FRAME_RTR;
        rx_frame.data_len = msg.dlc;
        memcpy(rx_frame.data, msg.data, 8);
        canardHandleRxFrame(&canard, &rx_frame, timestamp);
    }

    // transmit
    for (const CanardCANFrame* txf = NULL; (txf = canardPeekTxQueue(&canard)) != NULL;)
    {
        msg.ide = txf->id & CANARD_CAN_FRAME_EFF;
        msg.rtr = txf->id & CANARD_CAN_FRAME_RTR;
        msg.id = txf->id & CANARD_CAN_EXT_ID_MASK;
        msg.dlc = txf->data_len;
        memcpy(msg.data, txf->data, 8);

        bool success = canbus_send_message(&msg);

        if (success) {
            canardPopTxQueue(&canard);
        } else {
            break;
        }
    }
}

static struct uavcan_transfer_info_s get_transfer_info(const CanardInstance* ins, CanardRxTransfer* transfer)
{
    struct uavcan_transfer_info_s ret;
    ret.canardInstance = (void*)ins;
    ret.remote_node_id = transfer->source_node_id;
    ret.transfer_id = transfer->transfer_id;
    ret.priority = transfer->priority;
    return ret;
}

void uavcan_set_node_id(uint8_t node_id) {
    canardSetLocalNodeID(&canard, node_id);
}

uint8_t uavcan_get_node_id() {
    return canardGetLocalNodeID(&canard);
}

void uavcan_set_node_mode(enum uavcan_node_mode_t mode)
{
    node_mode = mode;
}

void uavcan_set_node_health(enum uavcan_node_health_t health)
{
    node_health = health;
}

void uavcan_set_uavcan_ready_cb(uavcan_ready_handler_ptr cb)
{
    uavcan_ready_cb = cb;
}


void uavcan_set_restart_cb(restart_handler_ptr cb)
{
    restart_cb = cb;
}

void uavcan_set_file_beginfirmwareupdate_cb(file_beginfirmwareupdate_handler_ptr cb)
{
    file_beginfirmwareupdate_cb = cb;
}

void uavcan_set_file_read_response_cb(file_read_response_handler_ptr cb)
{
    file_read_response_cb = cb;
}

void uavcan_set_node_info(struct uavcan_node_info_s new_node_info)
{
    node_info = new_node_info;
}

void uavcan_send_debug_key_value(const char* name, float val)
{
    size_t name_len = strlen(name);
    uint8_t msg_buf[UAVCAN_DEBUG_KEYVALUE_MESSAGE_MAX_SIZE];
    memcpy(&msg_buf[0], &val, sizeof(float));
    memcpy(&msg_buf[4], name, name_len);
    static uint8_t transfer_id;
    canardBroadcast(&canard, UAVCAN_DEBUG_KEYVALUE_DATA_TYPE_SIGNATURE, UAVCAN_DEBUG_KEYVALUE_DATA_TYPE_ID, &transfer_id, CANARD_TRANSFER_PRIORITY_LOWEST, msg_buf, sizeof(float)+name_len);
}

void uavcan_send_debug_logmessage(enum uavcan_loglevel_t log_level, const char* source, const char* text) {
    uint8_t msg_buf[UAVCAN_DEBUG_LOGMESSAGE_MESSAGE_MAX_SIZE];

    size_t source_len = strlen(source);
    size_t text_len = strlen(text);

    if (source_len > 31) {
        source_len = 31;
    }
    if (text_len > 90) {
        text_len = 90;
    }

    uint8_t source_len_uint8 = source_len;

    canardEncodeScalar(msg_buf, 0, 3, &log_level);
    canardEncodeScalar(msg_buf, 3, 5, &source_len_uint8);
    memcpy(&msg_buf[1], source, source_len);
    memcpy(&msg_buf[1+source_len], text, text_len);
    static uint8_t transfer_id;
    canardBroadcast(&canard, UAVCAN_DEBUG_LOGMESSAGE_DATA_TYPE_SIGNATURE, UAVCAN_DEBUG_LOGMESSAGE_DATA_TYPE_ID, &transfer_id, CANARD_TRANSFER_PRIORITY_LOWEST, msg_buf, 1+source_len+text_len);
}

// Node ID allocation - implementation of http://uavcan.org/Specification/figures/dynamic_node_id_allocatee_algorithm.svg
static void allocation_init(void)
{
    if (!allocation_running()) {
        return;
    }

    // Start request timer
    allocation_start_request_timer();
}

static void allocation_update(void)
{
    if (!allocation_running()) {
        return;
    }

    // Check allocation timer
    if (micros() - allocation_state.request_timer_begin_us > allocation_state.request_delay_us) {
        allocation_timer_expired();

    }
}

static void allocation_timer_expired(void)
{
    if (!allocation_running()) {
        return;
    }

    // Start allocation request timer
    allocation_start_request_timer();

    // Send allocation message
    uint8_t allocation_request[CANARD_CAN_FRAME_MAX_DATA_LEN - 1];
    uint8_t uid_size = MIN(UNIQUE_ID_LENGTH_BYTES-allocation_state.unique_id_offset, UAVCAN_NODE_ID_ALLOCATION_MAX_LENGTH_OF_UID_IN_REQUEST);
    allocation_request[0] = (allocation_state.unique_id_offset == 0) ? 1 : 0;
    memcpy(&allocation_request[1], &node_unique_id[allocation_state.unique_id_offset], uid_size);

    static uint8_t transfer_id;
    canardBroadcast(&canard, UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_SIGNATURE, UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_ID, &transfer_id, CANARD_TRANSFER_PRIORITY_LOW, allocation_request, uid_size+1);

    allocation_state.unique_id_offset = 0;
}

static void handle_allocation_data_broadcast(CanardInstance* ins, CanardRxTransfer* transfer)
{
    if (!allocation_running()) {
        return;
    }

    // Always start the allocation request timer and reset the UID offset
    allocation_start_request_timer();
    allocation_state.unique_id_offset = 0;

    if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID) {
        // If source node ID is anonymous, return
        return;
    }

    uint8_t received_unique_id[UNIQUE_ID_LENGTH_BYTES];
    uint8_t received_unique_id_len = transfer->payload_len-1;
    uint8_t i;
    for (i=0; i<received_unique_id_len; i++)
    {
        canardDecodeScalar(transfer, i*8+UAVCAN_NODE_ID_ALLOCATION_UID_BIT_OFFSET, 8, false, &received_unique_id[i]);
    }

    if(memcmp(node_unique_id, received_unique_id, received_unique_id_len) != 0)
    {
        // If unique ID does not match, return
        return;
    }

    if (received_unique_id_len < UNIQUE_ID_LENGTH_BYTES) {
        // Unique ID partially matches - set the UID offset and start the followup timer
        allocation_state.unique_id_offset = received_unique_id_len;
        allocation_start_followup_timer();
    } else {
        // Complete match received
        uint8_t allocated_node_id = 0;
        canardDecodeScalar(transfer, 0, 7, false, &allocated_node_id);
        if (allocated_node_id != 0) {
            canardSetLocalNodeID(ins, allocated_node_id);
        }
    }
}

static void allocation_start_request_timer(void)
{
    if (!allocation_running()) {
        return;
    }

    allocation_state.request_timer_begin_us = micros();
    allocation_state.request_delay_us = UAVCAN_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_US + (uint32_t)(getRandomFloat() * (UAVCAN_NODE_ID_ALLOCATION_MAX_REQUEST_PERIOD_US-UAVCAN_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_US));
}

static void allocation_start_followup_timer(void)
{
    if (!allocation_running()) {
        return;
    }

    allocation_state.request_timer_begin_us = micros();
    allocation_state.request_delay_us = UAVCAN_NODE_ID_ALLOCATION_MIN_FOLLOWUP_PERIOD_US + (uint32_t)(getRandomFloat() * (UAVCAN_NODE_ID_ALLOCATION_MAX_FOLLOWUP_PERIOD_US-UAVCAN_NODE_ID_ALLOCATION_MIN_FOLLOWUP_PERIOD_US));
}

static bool allocation_running(void)
{
    return canardGetLocalNodeID(&canard) == CANARD_BROADCAST_NODE_ID;
}

static void process1HzTasks(void)
{
    canardCleanupStaleTransfers(&canard, micros());

    {
        uint8_t buffer[UAVCAN_NODE_STATUS_MESSAGE_SIZE];
        makeNodeStatusMessage(buffer);

        static uint8_t transfer_id;

        canardBroadcast(&canard, UAVCAN_NODE_STATUS_DATA_TYPE_SIGNATURE, UAVCAN_NODE_STATUS_DATA_TYPE_ID, &transfer_id, CANARD_TRANSFER_PRIORITY_LOWEST, buffer, UAVCAN_NODE_STATUS_MESSAGE_SIZE);
    }
}

static void handle_get_node_info_request(CanardInstance* ins, CanardRxTransfer* transfer)
{
    uint8_t buffer[UAVCAN_GET_NODE_INFO_RESPONSE_MAX_SIZE];
    memset(buffer, 0, UAVCAN_GET_NODE_INFO_RESPONSE_MAX_SIZE);
    makeNodeStatusMessage(buffer);

    // Software Version
    buffer[7] = node_info.sw_major_version;
    buffer[8] = node_info.sw_minor_version;

    if (node_info.sw_vcs_commit_available) {
        buffer[9] |= 1; // set OPTIONAL_FIELD_FLAG_VCS_COMMIT
        canardEncodeScalar(buffer, 80, 32, &node_info.sw_vcs_commit);
    }

    if (node_info.sw_image_crc_available) {
        buffer[9] |= 2; // set OPTIONAL_FIELD_FLAG_IMAGE_CRC
        canardEncodeScalar(buffer, 112, 64, &node_info.sw_image_crc);
    }

    buffer[22] = node_info.hw_major_version;
    buffer[23] = node_info.hw_minor_version;

    // Unique ID
    memcpy(&buffer[24], node_unique_id, sizeof(node_unique_id));

    // Name
    const size_t name_len = strlen(node_info.hw_name);
    memcpy(&buffer[41], node_info.hw_name, name_len);

    const size_t total_size = 41 + name_len;

    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE, UAVCAN_GET_NODE_INFO_DATA_TYPE_ID, &transfer->transfer_id, transfer->priority, CanardResponse, buffer, total_size);
}

static void handle_restart_node_request(CanardInstance* ins, CanardRxTransfer* transfer)
{
    uint64_t magic;
    canardDecodeScalar(transfer, 0, 40, false, &magic);
    if (restart_cb) {
        restart_cb(get_transfer_info(ins, transfer), magic);
    }
}

void uavcan_send_restart_response(struct uavcan_transfer_info_s* transfer_info, bool ok)
{
    uint8_t resp_buf[UAVCAN_RESTARTNODE_RESPONSE_MAX_SIZE];
    canardEncodeScalar(resp_buf, 0, 1, &ok);

    canardRequestOrRespond(transfer_info->canardInstance, transfer_info->remote_node_id, UAVCAN_RESTARTNODE_DATA_TYPE_SIGNATURE, UAVCAN_RESTARTNODE_DATA_TYPE_ID, &transfer_info->transfer_id, transfer_info->priority, CanardResponse, resp_buf, UAVCAN_RESTARTNODE_RESPONSE_MAX_SIZE);
}

static void handle_file_beginfirmwareupdate_request(CanardInstance* ins, CanardRxTransfer* transfer)
{
    uint8_t source_node_id;
    canardDecodeScalar(transfer, 0, 8, false, &source_node_id);
    uint8_t path_len = transfer->payload_len-1;
    char path[201];

    for(uint8_t i=0; i<path_len; i++) {
        canardDecodeScalar(transfer, 8+i*8, 8, false, (uint8_t*)&path[i]);
    }
    path[path_len] = '\0';

    if (file_beginfirmwareupdate_cb) {
        file_beginfirmwareupdate_cb(get_transfer_info(ins, transfer), source_node_id, path);
    } else {
        struct uavcan_transfer_info_s transfer_info = get_transfer_info(ins, transfer);
        uavcan_send_file_beginfirmwareupdate_response(&transfer_info, UAVCAN_BEGINFIRMWAREUPDATE_ERROR_UNKNOWN, "");
    }
}

void uavcan_send_file_beginfirmwareupdate_response(struct uavcan_transfer_info_s* transfer_info, enum uavcan_beginfirmwareupdate_error_t error, const char* error_message)
{
    uint8_t buf[UAVCAN_FILE_BEGINFIRMWAREUPDATE_RESPONSE_MAX_SIZE];

    buf[0] = (uint8_t)error;
    size_t error_message_len = strlen(error_message);
    memcpy(&buf[1], error_message, error_message_len);

    size_t total_size = error_message_len+1;

    canardRequestOrRespond(transfer_info->canardInstance, transfer_info->remote_node_id, UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_SIGNATURE, UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_ID, &transfer_info->transfer_id, transfer_info->priority, CanardResponse, buf, total_size);
}


static uint8_t file_read_transfer_id;
uint8_t uavcan_send_file_read_request(uint8_t remote_node_id, const uint64_t offset, const char* path)
{
    uint8_t buf[UAVCAN_FILE_BEGINFIRMWAREUPDATE_RESPONSE_MAX_SIZE];

    canardEncodeScalar(buf, 0, 40, &offset);
    size_t path_len = strlen(path);
    memcpy(&buf[5], path, path_len);

    size_t total_size = path_len+5;

    uint8_t transfer_id = file_read_transfer_id;
    canardRequestOrRespond(&canard, remote_node_id, UAVCAN_FILE_READ_DATA_TYPE_SIGNATURE, UAVCAN_FILE_READ_DATA_TYPE_ID, &file_read_transfer_id, CANARD_TRANSFER_PRIORITY_LOWEST, CanardRequest, buf, total_size);

    return transfer_id;
}

static void handle_file_read_response(CanardInstance* ins, CanardRxTransfer* transfer)
{
    UNUSED(ins);
    int16_t error;
    uint8_t data[256];
    size_t data_len = transfer->payload_len-2;
    canardDecodeScalar(transfer, 0, 16, true, &error);

    for(uint16_t i=0; i<data_len; i++) {
        canardDecodeScalar(transfer, 16+i*8, 8, false, &data[i]);
    }

    if (file_read_response_cb) {
        file_read_response_cb(transfer->transfer_id, error, data, data_len, data_len<256);
    }
}

static void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer)
{
    if (transfer->transfer_type == CanardTransferTypeBroadcast && transfer->data_type_id == UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_ID) {
        handle_allocation_data_broadcast(ins, transfer);
    } else if (transfer->transfer_type == CanardTransferTypeRequest && transfer->data_type_id == UAVCAN_GET_NODE_INFO_DATA_TYPE_ID) {
        handle_get_node_info_request(ins, transfer);
    } else if (transfer->transfer_type == CanardTransferTypeRequest && transfer->data_type_id == UAVCAN_RESTARTNODE_DATA_TYPE_ID) {
        handle_restart_node_request(ins, transfer);
    } else if (transfer->transfer_type == CanardTransferTypeRequest && transfer->data_type_id == UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_ID) {
        handle_file_beginfirmwareupdate_request(ins, transfer);
    } else if (transfer->transfer_type == CanardTransferTypeResponse && transfer->data_type_id == UAVCAN_FILE_READ_DATA_TYPE_ID) {
        handle_file_read_response(ins, transfer);
    }
}

static void makeNodeStatusMessage(uint8_t* buffer)
{
    memset(buffer, 0, UAVCAN_NODE_STATUS_MESSAGE_SIZE);

    if (started_at_sec == 0) {
        started_at_sec = millis()/1000U;
    }

    const uint32_t uptime_sec = millis()/1000U - started_at_sec;

    canardEncodeScalar(buffer,  0, 32, &uptime_sec);
    canardEncodeScalar(buffer, 32,  2, &node_health);
    canardEncodeScalar(buffer, 34,  3, &node_mode);
}

static bool shouldAcceptTransfer(const CanardInstance* ins, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id)
{
    UNUSED(ins);
    UNUSED(source_node_id);
    if (allocation_running())
    {
        if ((transfer_type == CanardTransferTypeBroadcast) &&
            (data_type_id == UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_ID))
        {
            *out_data_type_signature = UAVCAN_NODE_ID_ALLOCATION_DATA_TYPE_SIGNATURE;
            return true;
        }
        return false;
    }

    if (transfer_type == CanardTransferTypeRequest && data_type_id == UAVCAN_GET_NODE_INFO_DATA_TYPE_ID)
    {
        *out_data_type_signature = UAVCAN_GET_NODE_INFO_DATA_TYPE_SIGNATURE;
        return true;
    }

    if (transfer_type == CanardTransferTypeRequest && data_type_id == UAVCAN_RESTARTNODE_DATA_TYPE_ID)
    {
        *out_data_type_signature = UAVCAN_RESTARTNODE_DATA_TYPE_SIGNATURE;
        return true;
    }

    if (transfer_type == CanardTransferTypeRequest && data_type_id == UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_ID)
    {
        *out_data_type_signature = UAVCAN_FILE_BEGINFIRMWAREUPDATE_DATA_TYPE_SIGNATURE;
        return true;
    }

    if (transfer_type == CanardTransferTypeResponse && data_type_id == UAVCAN_FILE_READ_DATA_TYPE_ID)
    {
        *out_data_type_signature = UAVCAN_FILE_READ_DATA_TYPE_SIGNATURE;
        return true;
    }

    return false;
}

static float getRandomFloat(void)
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        board_get_unique_id((uint32_t*)&node_unique_id[0]);

        const uint32_t* unique_32 = (uint32_t*)&node_unique_id[0];

        srand(micros() ^ *unique_32);
    }

    return (float)rand() / (float)RAND_MAX;
}

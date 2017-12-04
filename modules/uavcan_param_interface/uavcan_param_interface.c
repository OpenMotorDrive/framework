#include <modules/uavcan/uavcan.h>
#include <modules/pubsub/pubsub.h>
#include <common/ctor.h>
#include <modules/param/param.h>
#include <string.h>
#include <modules/worker_thread/worker_thread.h>

#ifndef UAVCAN_PARAM_INTERFACE_WORKER_THREAD
#error Please define UAVCAN_PARAM_INTERFACE_WORKER_THREAD in worker_threads_conf.h.
#endif

#define WT UAVCAN_PARAM_INTERFACE_WORKER_THREAD
WORKER_THREAD_DECLARE_EXTERN(WT)

#include <uavcan.protocol.param.GetSet.h>
#include <uavcan.protocol.param.ExecuteOpcode.h>

static struct worker_thread_listener_task_s getset_req_listener_task;
static void getset_req_handler(size_t msg_size, const void* buf, void* ctx);

static struct worker_thread_listener_task_s opcode_req_listener_task;
static void opcode_req_handler(size_t msg_size, const void* buf, void* ctx);

RUN_AFTER(UAVCAN_INIT) {
    struct pubsub_topic_s* getset_req_topic = uavcan_get_message_topic(0, &uavcan_protocol_param_GetSet_req_descriptor);
    worker_thread_add_listener_task(&WT, &getset_req_listener_task, getset_req_topic, getset_req_handler, NULL);

    struct pubsub_topic_s* opcode_req_topic = uavcan_get_message_topic(0, &uavcan_protocol_param_ExecuteOpcode_req_descriptor);
    worker_thread_add_listener_task(&WT, &opcode_req_listener_task, opcode_req_topic, opcode_req_handler, NULL);
}

static void getset_req_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_param_GetSet_req_s* req = (const struct uavcan_protocol_param_GetSet_req_s*)msg_wrapper->msg;

    struct uavcan_protocol_param_GetSet_res_s res;
    memset(&res, 0, sizeof(struct uavcan_protocol_param_GetSet_res_s));

    param_acquire();

    int16_t param_idx = req->index;
    if (req->name_len > 0) {
        // If the name field is not empty, we are to prefer it over the param index field
        param_idx = param_get_index_by_name(req->name_len, (char*)req->name);
    }

    if (param_get_exists(param_idx)) {
        switch(req->value.uavcan_protocol_param_Value_type) {
            case UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_INTEGER_VALUE: {
                // set request: int64
                param_set_by_index_integer(param_idx, req->value.integer_value);
                break;
            }
            case UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_REAL_VALUE: {
                // set request: float
                param_set_by_index_float32(param_idx, req->value.real_value);
                break;
            }
            case UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_BOOLEAN_VALUE: {
                // set request: bool
                param_set_by_index_bool(param_idx, req->value.boolean_value);
                break;
            }
            case UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_STRING_VALUE: {
                // set request: string
                param_set_by_index_string(param_idx, req->value.string_value_len, (char*)req->value.string_value);
                break;
            }
            default:
                break;
        }

        const char* param_name = param_get_name_by_index(param_idx);
        res.name_len = strnlen(param_name,92);
        memcpy(res.name, param_name, res.name_len);

        switch(param_get_type_by_index(param_idx)) {
            case PARAM_TYPE_STRING:
                res.value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_STRING_VALUE;
                param_get_value_by_index_string(param_idx, &res.value.string_value_len, (char*)res.value.string_value, sizeof(res.value.string_value));

                res.default_value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_STRING_VALUE;
                param_get_default_value_by_index_string(param_idx, &res.default_value.string_value_len, (char*)res.default_value.string_value, sizeof(res.default_value.string_value));

                res.max_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_EMPTY;
                res.min_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_EMPTY;
                break;
            case PARAM_TYPE_BOOL:
                res.value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_BOOLEAN_VALUE;
                param_get_value_by_index_bool(param_idx, (bool*)&res.value.boolean_value);

                res.default_value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_BOOLEAN_VALUE;
                param_get_default_value_by_index_bool(param_idx, (bool*)&res.default_value.boolean_value);

                res.max_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_EMPTY;
                res.min_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_EMPTY;
                break;
            case PARAM_TYPE_FLOAT32:
                res.value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_REAL_VALUE;
                param_get_value_by_index_float32(param_idx, &res.value.real_value);

                res.default_value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_REAL_VALUE;
                param_get_default_value_by_index_float32(param_idx, &res.default_value.real_value);

                res.max_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_REAL_VALUE;
                param_get_max_value_by_index_float32(param_idx, &res.max_value.real_value);

                res.min_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_REAL_VALUE;
                param_get_min_value_by_index_float32(param_idx, &res.min_value.real_value);
                break;
            case PARAM_TYPE_UINT32:
            case PARAM_TYPE_UINT16:
            case PARAM_TYPE_UINT8:
            case PARAM_TYPE_INT64:
            case PARAM_TYPE_INT32:
            case PARAM_TYPE_INT16:
            case PARAM_TYPE_INT8:
                res.value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_INTEGER_VALUE;
                param_get_value_by_index_integer(param_idx, &res.value.integer_value);

                res.default_value.uavcan_protocol_param_Value_type = UAVCAN_PROTOCOL_PARAM_VALUE_TYPE_INTEGER_VALUE;
                param_get_default_value_by_index_integer(param_idx, &res.default_value.integer_value);

                res.max_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_INTEGER_VALUE;
                param_get_max_value_by_index_integer(param_idx, &res.max_value.integer_value);

                res.min_value.uavcan_protocol_param_NumericValue_type = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_TYPE_INTEGER_VALUE;
                param_get_min_value_by_index_integer(param_idx, &res.min_value.integer_value);
                break;
            default:
                break;
        }

    }

    param_release();

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

static void opcode_req_handler(size_t msg_size, const void* buf, void* ctx) {
    (void)msg_size;
    (void)ctx;

    const struct uavcan_deserialized_message_s* msg_wrapper = buf;
    const struct uavcan_protocol_param_ExecuteOpcode_req_s* req = (const struct uavcan_protocol_param_ExecuteOpcode_req_s*)msg_wrapper->msg;

    struct uavcan_protocol_param_ExecuteOpcode_res_s res;
    memset(&res, 0, sizeof(struct uavcan_protocol_param_ExecuteOpcode_res_s));

    switch(req->opcode) {
        case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQ_OPCODE_SAVE:
            param_acquire();
            res.ok = param_store_all();
            param_release();
            break;
        case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQ_OPCODE_ERASE:
            param_acquire();
            res.ok = param_erase();
            param_release();
            break;
    }

    uavcan_respond(msg_wrapper->uavcan_idx, msg_wrapper, &res);
}

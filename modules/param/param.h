#pragma once
#include <common/ctor.h>
#include <uavcan/uavcan.h>
#include "flash_journal.h"
#include <stdint.h>
#include <stdbool.h>

#define __PARAM_CONCAT(a,b) a ## b
#define _PARAM_CONCAT(a,b) __PARAM_CONCAT(a,b)

#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_FLOAT32 param_descriptor_float32_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_UINT32 param_descriptor_uint32_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_UINT16 param_descriptor_uint16_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_UINT8 param_descriptor_uint8_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_INT64 param_descriptor_int64_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_INT32 param_descriptor_int32_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_INT16 param_descriptor_int16_s
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_PARAM_TYPE_INT8 param_descriptor_int8_s
#define __PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE) _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME_##PARAM_TYPE
#define _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE) __PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE)

#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_FLOAT32 float
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_UINT32 uint32_t
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_UINT16 uint16_t
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_UINT8 uint8_t
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_INT64 int64_t
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_INT32 int32_t
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_INT16 int16_t
#define _PARAM_SCALAR_CTYPE_PARAM_TYPE_INT8 int8_t
#define __PARAM_SCALAR_CTYPE(PARAM_TYPE) _PARAM_SCALAR_CTYPE_##PARAM_TYPE
#define _PARAM_SCALAR_CTYPE(PARAM_TYPE) __PARAM_SCALAR_CTYPE(PARAM_TYPE)

#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_UINT32 0
#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_UINT16 0
#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_UINT8 0
#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_INT64 INT64_MIN
#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_INT32 INT32_MIN
#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_INT16 INT16_MIN
#define _PARAM_INTEGER_MIN_VALUE_PARAM_TYPE_INT8 INT8_MIN
#define __PARAM_INTEGER_MIN_VALUE(PARAM_TYPE) _PARAM_INTEGER_MIN_VALUE_##PARAM_TYPE
#define _PARAM_INTEGER_MIN_VALUE(PARAM_TYPE) __PARAM_INTEGER_MIN_VALUE(PARAM_TYPE)

#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_UINT32 UINT32_MAX
#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_UINT16 UINT16_MAX
#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_UINT8 UINT8_MAX
#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_INT64 INT64_MAX
#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_INT32 INT32_MAX
#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_INT16 INT16_MAX
#define _PARAM_INTEGER_MAX_VALUE_PARAM_TYPE_INT8 INT8_MAX
#define __PARAM_INTEGER_MAX_VALUE(PARAM_TYPE) _PARAM_INTEGER_MAX_VALUE_##PARAM_TYPE
#define _PARAM_INTEGER_MAX_VALUE(PARAM_TYPE) __PARAM_INTEGER_MAX_VALUE(PARAM_TYPE)

#define _PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE) \
struct __attribute__((packed)) _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE) { \
    struct param_descriptor_header_s header; \
    _PARAM_SCALAR_CTYPE(PARAM_TYPE) default_val; \
    _PARAM_SCALAR_CTYPE(PARAM_TYPE) min_val; \
    _PARAM_SCALAR_CTYPE(PARAM_TYPE) max_val; \
};

#define _PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE) \
static _PARAM_SCALAR_CTYPE(PARAM_TYPE) HANDLE_NAME; \
static const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE) _PARAM_CONCAT(_param_local_descriptor_structure_, HANDLE_NAME) = {{PARAM_TYPE, 0, NAME, &HANDLE_NAME}, DEFAULT_VAL, MIN_VAL, MAX_VAL}; \
RUN_AFTER(PARAM_INIT) { \
    param_register((const struct param_descriptor_header_s*)& _PARAM_CONCAT(_param_local_descriptor_structure_, HANDLE_NAME)); \
}

#define PARAM_DEFINE_BOOL_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL) \
static bool HANDLE_NAME; \
static const struct param_descriptor_bool_s _PARAM_CONCAT(_param_local_descriptor_structure_, HANDLE_NAME) = {{PARAM_TYPE_BOOL, DEFAULT_VAL, NAME, &HANDLE_NAME}}; \
RUN_AFTER(PARAM_INIT) { \
    param_register((const struct param_descriptor_header_s*)& _PARAM_CONCAT(_param_local_descriptor_structure_, HANDLE_NAME)); \
}

#define PARAM_DEFINE_STRING_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MAX_LEN) \
static char HANDLE_NAME[MAX_LEN+1]; \
static const struct param_descriptor_string_s _PARAM_CONCAT(_param_local_descriptor_structure_, HANDLE_NAME) = {{PARAM_TYPE_STRING, 0, NAME, HANDLE_NAME}, MAX_LEN, DEFAULT_VAL}; \
RUN_AFTER(PARAM_INIT) { \
    param_register((const struct param_descriptor_header_s*)& _PARAM_CONCAT(_param_local_descriptor_structure_, HANDLE_NAME)); \
}

#define PARAM_DEFINE_FLOAT32_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_FLOAT32)

#define PARAM_DEFINE_UINT32_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_UINT32)

#define PARAM_DEFINE_UINT16_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_UINT16)

#define PARAM_DEFINE_UINT8_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_UINT8)

#define PARAM_DEFINE_INT64_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_INT64)

#define PARAM_DEFINE_INT32_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_INT32)

#define PARAM_DEFINE_INT16_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_INT16)

#define PARAM_DEFINE_INT8_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL) \
_PARAM_DEFINE_SCALAR_PARAM_STATIC(HANDLE_NAME, NAME, DEFAULT_VAL, MIN_VAL, MAX_VAL, PARAM_TYPE_INT8)

enum param_type_t {
    PARAM_TYPE_FLOAT32 = 0,
    PARAM_TYPE_UINT32,
    PARAM_TYPE_UINT16,
    PARAM_TYPE_UINT8,
    PARAM_TYPE_INT64,
    PARAM_TYPE_INT32,
    PARAM_TYPE_INT16,
    PARAM_TYPE_INT8,
    PARAM_TYPE_BOOL,
    PARAM_TYPE_STRING,
    PARAM_NUM_TYPES,
    PARAM_TYPE_UNKNOWN
};

struct __attribute__((packed)) param_descriptor_header_s {
    uint8_t type:7;
    uint8_t bool_default_value:1;
    const char* name;
    void* cached_value;
};


_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_FLOAT32)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_UINT32)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_UINT16)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_UINT8)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_INT64)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_INT32)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_INT16)
_PARAM_DEFINE_SCALAR_DESCRIPTOR_STRUCT(PARAM_TYPE_INT8)

struct __attribute__((packed)) param_descriptor_string_s {
    struct param_descriptor_header_s header;
    uint8_t max_len;
    const char* default_val;
};

struct __attribute__((packed)) param_descriptor_bool_s {
    struct param_descriptor_header_s header;
};

void param_acquire(void);
void param_release(void);
void param_register(const struct param_descriptor_header_s* param_descriptor_header);
uint16_t param_get_num_params_registered(void);
bool param_get_exists(uint16_t param_idx);
enum param_type_t param_get_type_by_index(uint16_t param_idx);
int16_t param_get_index_by_name(uint8_t name_len, char* name);
const char* param_get_name_by_index(uint16_t param_idx);

void param_set_by_index_integer(uint16_t param_idx, int64_t value);
void param_set_by_index_float32(uint16_t param_idx, float value);
void param_set_by_index_bool(uint16_t param_idx, bool value);
void param_set_by_index_string(uint16_t param_idx, uint8_t length, const char* value);

bool param_get_value_by_index_integer(uint16_t param_idx, int64_t* value);
bool param_get_value_by_index_float32(uint16_t param_idx, float* value);
bool param_get_value_by_index_bool(uint16_t param_idx, bool* value);
bool param_get_value_by_index_string(uint16_t param_idx, uint8_t* length, char* value, size_t value_size);

bool param_get_default_value_by_index_integer(uint16_t param_idx, int64_t* value);
bool param_get_default_value_by_index_float32(uint16_t param_idx, float* value);
bool param_get_default_value_by_index_bool(uint16_t param_idx, bool* value);
bool param_get_default_value_by_index_string(uint16_t param_idx, uint8_t* length, char* value, size_t value_size);

bool param_get_max_value_by_index_integer(uint16_t param_idx, int64_t* value);
bool param_get_max_value_by_index_float32(uint16_t param_idx, float* value);

bool param_get_min_value_by_index_integer(uint16_t param_idx, int64_t* value);
bool param_get_min_value_by_index_float32(uint16_t param_idx, float* value);

bool param_erase(void);
bool param_store_all(void);

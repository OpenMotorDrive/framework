#include <common/param.h>
#include <common/helpers.h>

#include <ch.h>

#include <string.h>
#include <stdio.h>

#define PARAM_MAX_NUM_PARAMS 50
#define PARAM_STORAGE_SIZE 1024

#define PARAM_FORMAT_VERSION 1

// NOTE: This parameter system uses a 40-bit hash as a key. The chance of a collision occurring
// among N keys is roughly k^2/2199023255552 - approximately 1 in 10 million for 500 keys. A
// check will be performed for collisions despite this.
struct __attribute__((packed)) param_key_s {
    uint8_t bytes[5];
};

struct __attribute__((packed)) param_journal_header_s {
    uint8_t format_version;
    int8_t index;
    uint16_t initial_entry_count;
};

struct __attribute__((packed)) param_journal_key_value_s {
    struct param_key_s key;
    uint8_t value[];
};

struct flash_journal_instance_s journals[2];
static struct flash_journal_instance_s* active_journal;

static const struct param_descriptor_header_s* param_descriptor_table[PARAM_MAX_NUM_PARAMS];
static struct param_key_s param_keys[PARAM_MAX_NUM_PARAMS];

static uint16_t num_params_registered;
MUTEX_DECL(param_mutex);

static const struct param_journal_header_s* param_get_journal_header_ptr(struct flash_journal_instance_s* journal);
static void param_load_cache_value_from_journal(uint16_t param_idx);
static void param_load_cache_value_from_hard_coded_default(uint16_t param_idx);
static void param_load_cache_value_from_all(uint16_t param_idx);
static const struct flash_journal_entry_s* param_get_final_journal_entry_with_key(const struct param_key_s* key);
static bool param_write_to_flash_journal(const struct param_key_s* key, size_t value_size, const void* value);
static bool param_store_by_idx(uint16_t param_idx);
static void param_generate_key(uint8_t name_len, const char* name, const enum param_type_t param_type, struct param_key_s* ret);
static uint8_t param_get_data_max_size(uint16_t param_idx);
static bool param_type_get_size_fixed(enum param_type_t type);
static int64_t param_uncompress_varint64(uint8_t len, const uint8_t* buf);
static uint8_t param_compress_varint64(int64_t value, uint8_t* buf);
static bool param_journal_iterate_final_values(const struct flash_journal_entry_s** iterator);
static void param_compress_journal(void);

void param_init(void) {
    param_acquire();

    flash_journal_init(&journals[0], BOARD_PARAM1_ADDR, BOARD_PARAM1_FLASH_SIZE);
    flash_journal_init(&journals[1], BOARD_PARAM2_ADDR, BOARD_PARAM2_FLASH_SIZE);

    for (uint8_t i=0; i<2; i++) {
        const struct param_journal_header_s* header = param_get_journal_header_ptr(&journals[i]);

        if (!header || flash_journal_count_entries(&journals[i]) < header->initial_entry_count || header->format_version != PARAM_FORMAT_VERSION) {
            // journals[i] is invalid - continue
            continue;
        }

        if (!active_journal || (header->index - param_get_journal_header_ptr(active_journal)->index) > 0) {
            active_journal = &journals[i];
        }
    }

    if (!active_journal) {
        // no valid journal was found - initialize an empty one
        struct param_journal_header_s header = {PARAM_FORMAT_VERSION, 0, 1};

        flash_journal_erase(&journals[0]);
        flash_journal_write(&journals[0], sizeof(header), &header);

        active_journal = &journals[0];
    }

    // Compress journal if needed
    param_compress_journal();

    for (uint16_t i=0; i<num_params_registered; i++) {
        param_load_cache_value_from_all(i);
    }

    param_release();
}

void _param_register(const struct param_descriptor_header_s* param_descriptor_header) {
    if (num_params_registered < PARAM_MAX_NUM_PARAMS) {
        uint16_t new_param_idx = num_params_registered;

        param_generate_key(strnlen(param_descriptor_header->name, 92), param_descriptor_header->name, param_descriptor_header->type, &param_keys[new_param_idx]);

        for (uint32_t i=0; i<new_param_idx; i++) {
            if(!memcmp(&param_keys[i], &param_keys[new_param_idx], sizeof(struct param_key_s))) {
                // Double registration or hash collision
                return;
            }
        }

        param_descriptor_table[new_param_idx] = param_descriptor_header;

        num_params_registered++;

        param_load_cache_value_from_all(new_param_idx);
    }
}

void param_register(const struct param_descriptor_header_s* param_descriptor_header) {
    param_acquire();

    _param_register(param_descriptor_header);

    param_release();
}

bool param_erase(void) {
    struct param_journal_header_s header = {PARAM_FORMAT_VERSION, 0, 1};

    flash_journal_erase((active_journal == &journals[0]) ? &journals[1] : &journals[0]);
    flash_journal_erase((active_journal == &journals[0]) ? &journals[0] : &journals[1]);

    flash_journal_write(&journals[0], sizeof(header), &header);
    active_journal = &journals[0];
    return true;
}

bool param_store_all(void) {
    for (uint16_t i=0; i<num_params_registered; i++) {
        if (!param_store_by_idx(i)) {
            param_compress_journal();
            if (!param_store_by_idx(i)) {
                return false;
            }
        }
    }
    return true;
}

uint16_t param_get_num_params_registered(void) {
    return num_params_registered;
}

int16_t param_get_index_by_name(uint8_t name_len, char* name) {
    for (uint16_t i=0; i<num_params_registered; i++) {
        const struct param_descriptor_header_s* descriptor = param_descriptor_table[i];

        if(!strncmp(name, descriptor->name, MIN(name_len,92))) {
            return i;
        }
    }

    return -1;
}

void param_set_by_index_string(uint16_t param_idx, uint8_t length, const char* value) {
    if (param_idx >= num_params_registered || !value) {
        return;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    if (descriptor->type == PARAM_TYPE_STRING) {
        const struct param_descriptor_string_s* string_descriptor = (const struct param_descriptor_string_s*)descriptor;
        size_t max_len = MIN(string_descriptor->max_len,128);

        if (length <= max_len) {
            strncpy(descriptor->cached_value, value, max_len);
        }
    }
}

void param_set_by_index_bool(uint16_t param_idx, bool value) {
    if (param_idx >= num_params_registered) {
        return;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    if (descriptor->type == PARAM_TYPE_BOOL) {
        *(bool*)descriptor->cached_value = value;
    }
}

void param_set_by_index_float32(uint16_t param_idx, float value) {
    if (param_idx >= num_params_registered) {
        return;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    if (descriptor->type == PARAM_TYPE_FLOAT32) {
        *(_PARAM_SCALAR_CTYPE(PARAM_TYPE_FLOAT32)*)descriptor->cached_value = value;
    }
}

void param_set_by_index_integer(uint16_t param_idx, int64_t value) {
    if (param_idx >= num_params_registered) {
        return;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    switch(descriptor->type) {
        #define PARAM_INTEGER_CASE(PARAM_TYPE) \
        case PARAM_TYPE: {\
            if (value >= _PARAM_INTEGER_MIN_VALUE(PARAM_TYPE) && value <= _PARAM_INTEGER_MAX_VALUE(PARAM_TYPE)) { \
                *(_PARAM_SCALAR_CTYPE(PARAM_TYPE)*)descriptor->cached_value = value; \
            } \
            break; \
        }
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT32)
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT16)
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT8)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT64)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT32)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT16)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT8)
        #undef PARAM_INTEGER_CASE
    }
}

void param_make_uavcan_getset_response(uint16_t param_idx, struct uavcan_param_getset_response_s* response) {
    if (param_idx >= num_params_registered || !response) {
        return;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    response->name_len = strnlen(descriptor->name, sizeof(response->name));
    memcpy(response->name, descriptor->name, response->name_len);

    switch(descriptor->type) {
        case PARAM_TYPE_FLOAT32: {
            response->value.type = UAVCAN_PARAM_VALUE_TYPE_FLOAT32;
            response->value.real_value = *(_PARAM_SCALAR_CTYPE(PARAM_TYPE_FLOAT32)*)descriptor->cached_value;
            response->default_value.type = UAVCAN_PARAM_VALUE_TYPE_FLOAT32;
            response->default_value.real_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE_FLOAT32)*)descriptor)->default_val;
            response->max_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_FLOAT32;
            response->max_value.real_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE_FLOAT32)*)descriptor)->max_val;
            response->min_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_FLOAT32;
            response->min_value.real_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE_FLOAT32)*)descriptor)->min_val;
            break;
        }

        case PARAM_TYPE_BOOL: {
            response->value.type = UAVCAN_PARAM_VALUE_TYPE_BOOL;
            response->value.boolean_value = *(bool*)descriptor->cached_value;
            response->default_value.type = UAVCAN_PARAM_VALUE_TYPE_BOOL;
            response->default_value.boolean_value = descriptor->bool_default_value;
            response->max_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_EMPTY;
            response->min_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_EMPTY;
            break;
        }

        case PARAM_TYPE_STRING: {
            const struct param_descriptor_string_s* string_descriptor = (const struct param_descriptor_string_s*)descriptor;
            size_t max_len = MIN(MIN(sizeof(response->value.string_value), string_descriptor->max_len),128);

            response->value.type = UAVCAN_PARAM_VALUE_TYPE_STRING;
            response->value.string_value_len = strnlen(descriptor->cached_value, max_len);
            memcpy(response->value.string_value, descriptor->cached_value, response->value.string_value_len);

            response->default_value.type = UAVCAN_PARAM_VALUE_TYPE_STRING;
            response->default_value.string_value_len = strnlen(string_descriptor->default_val, max_len);
            memcpy(response->default_value.string_value, string_descriptor->default_val, response->default_value.string_value_len);

            response->max_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_EMPTY;
            response->min_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_EMPTY;
            break;
        }

        #define PARAM_INTEGER_CASE(PARAM_TYPE) \
        case PARAM_TYPE: {\
            response->value.type = UAVCAN_PARAM_VALUE_TYPE_INT64; \
            response->value.integer_value = *(_PARAM_SCALAR_CTYPE(PARAM_TYPE)*)descriptor->cached_value; \
            response->default_value.type = UAVCAN_PARAM_VALUE_TYPE_INT64; \
            response->default_value.integer_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE)*)descriptor)->default_val; \
            response->max_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_INT64; \
            response->max_value.integer_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE)*)descriptor)->max_val; \
            response->min_value.type = UAVCAN_PARAM_NUMERICVALUE_TYPE_INT64; \
            response->min_value.integer_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE)*)descriptor)->min_val; \
            break; \
        }
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT32)
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT16)
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT8)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT64)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT32)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT16)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT8)
        #undef PARAM_INTEGER_CASE
    }
}

static bool param_store_by_idx(uint16_t param_idx) {
    if (param_idx >= num_params_registered) {
        return false;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    bool ret = false;

    switch(descriptor->type) {
        #define PARAM_INTEGER_CASE(PARAM_TYPE) \
        case PARAM_TYPE: { \
            uint8_t varint64_buf[sizeof(uint64_t)]; \
            size_t param_data_size = param_compress_varint64(*(_PARAM_SCALAR_CTYPE(PARAM_TYPE)*)descriptor->cached_value, varint64_buf);\
            ret = param_write_to_flash_journal(&param_keys[param_idx], param_data_size, varint64_buf);\
            break; \
        }
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT32)
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT16)
        PARAM_INTEGER_CASE(PARAM_TYPE_UINT8)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT64)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT32)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT16)
        PARAM_INTEGER_CASE(PARAM_TYPE_INT8)
        #undef PARAM_INTEGER_CASE
        case PARAM_TYPE_STRING: {
            size_t param_data_max_size = param_get_data_max_size(param_idx);
            size_t param_data_size = strnlen(descriptor->cached_value, param_data_max_size);
            ret = param_write_to_flash_journal(&param_keys[param_idx], param_data_size, descriptor->cached_value);
            break;
        }
        case PARAM_TYPE_FLOAT32:
        case PARAM_TYPE_BOOL: {
            size_t param_data_size = param_get_data_max_size(param_idx);
            ret = param_write_to_flash_journal(&param_keys[param_idx], param_data_size, descriptor->cached_value);
            break;
        }
    }

    return ret;
}

static bool param_write_to_flash_journal(const struct param_key_s* key, size_t value_size, const void* value) {
    if (!active_journal) {
        return false;
    }

    const void* existing_stored_param_value = NULL;
    size_t existing_stored_param_value_size;

    // Find the existing entry for this param, if it exists
    {
        const struct flash_journal_entry_s* existing_journal_entry = param_get_final_journal_entry_with_key(key);
        if (existing_journal_entry) {
            const struct param_journal_key_value_s* stored_param_key_value = (const struct param_journal_key_value_s*)(existing_journal_entry->data);
            existing_stored_param_value = stored_param_key_value->value;
            existing_stored_param_value_size = existing_journal_entry->len;
        }
    }

    if (existing_stored_param_value && existing_stored_param_value_size == value_size && !memcmp(existing_stored_param_value, value, value_size)) {
        return true;
    }

    return flash_journal_write_from_2_buffers(active_journal, sizeof(struct param_key_s), key, value_size, value);
}

static void param_compress_journal(void) {
    if (!active_journal) {
        return;
    }

    // Count unique entries
    uint32_t unique_param_entry_count = 0;
    const struct flash_journal_entry_s* journal_entry = NULL;
    while(param_journal_iterate_final_values(&journal_entry)) {
        unique_param_entry_count++;
    }

    if (unique_param_entry_count+1 == flash_journal_count_entries(active_journal)) {
        // Compression not needed
        return;
    }

    struct flash_journal_instance_s* inactive_journal = (active_journal == &journals[0]) ? &journals[1] : &journals[0];

    // Write header
    {
        // NOTE: initial_entry_count includes the header
        struct param_journal_header_s header = {
            PARAM_FORMAT_VERSION,
            param_get_journal_header_ptr(active_journal)->index + 1,
            unique_param_entry_count + 1
        };

        flash_journal_erase(inactive_journal);
        flash_journal_write(inactive_journal, sizeof(header), &header);
    }

    // Write entries
    journal_entry = NULL;
    while(param_journal_iterate_final_values(&journal_entry)) {
        flash_journal_write(inactive_journal, journal_entry->len, journal_entry->data);
    }

    // Switch journals
    active_journal = inactive_journal;
}

static void param_load_cache_value_from_hard_coded_default(uint16_t param_idx) {
    if (param_idx >= num_params_registered) {
        return;
    }

    const struct param_descriptor_header_s* descriptor_header = param_descriptor_table[param_idx];

    uint8_t param_data_max_size = param_get_data_max_size(param_idx);
    switch(descriptor_header->type) {
        #define PARAM_SCALAR_CASE(PARAM_TYPE) \
        case PARAM_TYPE: \
            *(_PARAM_SCALAR_CTYPE(PARAM_TYPE)*)descriptor_header->cached_value = ((const struct _PARAM_SCALAR_DESCRIPTOR_STRUCT_NAME(PARAM_TYPE)*)descriptor_header)->default_val; \
            break;

        PARAM_SCALAR_CASE(PARAM_TYPE_FLOAT32)
        PARAM_SCALAR_CASE(PARAM_TYPE_UINT32)
        PARAM_SCALAR_CASE(PARAM_TYPE_UINT16)
        PARAM_SCALAR_CASE(PARAM_TYPE_UINT8)
        PARAM_SCALAR_CASE(PARAM_TYPE_INT64)
        PARAM_SCALAR_CASE(PARAM_TYPE_INT32)
        PARAM_SCALAR_CASE(PARAM_TYPE_INT16)
        PARAM_SCALAR_CASE(PARAM_TYPE_INT8)
        #undef PARAM_SCALAR_CASE
        case PARAM_TYPE_BOOL:
            *(bool*)descriptor_header->cached_value = descriptor_header->bool_default_value;
            break;
        case PARAM_TYPE_STRING: {
            const struct param_descriptor_string_s* string_descriptor = (const struct param_descriptor_string_s*)descriptor_header;
            strncpy(descriptor_header->cached_value, string_descriptor->default_val, param_data_max_size-1);
            ((char*)descriptor_header->cached_value)[param_data_max_size] = '\0';
            break;
        }
    }
}

static void param_load_cache_value_from_journal(uint16_t param_idx) {
    if (!active_journal || param_idx >= num_params_registered) {
        return;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];
    const struct param_key_s* key = &param_keys[param_idx];

    const struct flash_journal_entry_s* journal_entry = param_get_final_journal_entry_with_key(key);
    if (journal_entry) {
        const struct param_journal_key_value_s* stored_param_key_value = (const struct param_journal_key_value_s*)(journal_entry->data);
        uint8_t stored_param_value_size = journal_entry->len-sizeof(struct param_key_s);
        uint8_t param_data_max_size = param_get_data_max_size(param_idx);
        bool param_type_size_fixed = param_type_get_size_fixed(descriptor->type);

        if ((param_type_size_fixed && stored_param_value_size == param_data_max_size) || (!param_type_size_fixed && stored_param_value_size <= param_data_max_size)) {
            switch(descriptor->type) {
                #define PARAM_INTEGER_CASE(PARAM_TYPE) \
                case PARAM_TYPE: \
                    *((_PARAM_SCALAR_CTYPE(PARAM_TYPE)*)descriptor->cached_value) = param_uncompress_varint64(stored_param_value_size, stored_param_key_value->value); \
                    break;

                PARAM_INTEGER_CASE(PARAM_TYPE_UINT32)
                PARAM_INTEGER_CASE(PARAM_TYPE_UINT16)
                PARAM_INTEGER_CASE(PARAM_TYPE_UINT8)
                PARAM_INTEGER_CASE(PARAM_TYPE_INT64)
                PARAM_INTEGER_CASE(PARAM_TYPE_INT32)
                PARAM_INTEGER_CASE(PARAM_TYPE_INT16)
                PARAM_INTEGER_CASE(PARAM_TYPE_INT8)
                #undef PARAM_INTEGER_CASE
                case PARAM_TYPE_FLOAT32:
                case PARAM_TYPE_BOOL:
                    memcpy(descriptor->cached_value, stored_param_key_value->value, stored_param_value_size);
                    break;
                case PARAM_TYPE_STRING:
                    memcpy(descriptor->cached_value, stored_param_key_value->value, stored_param_value_size);
                    ((uint8_t*)descriptor->cached_value)[stored_param_value_size] = 0; // write null-terminator
                    break;
            }
        }
    }
}

static const struct flash_journal_entry_s* param_get_final_journal_entry_with_key(const struct param_key_s* key) {
    if (!active_journal) {
        return NULL;
    }

    const struct flash_journal_entry_s* journal_entry = NULL;
    while(param_journal_iterate_final_values(&journal_entry)) {
        const struct param_journal_key_value_s* stored_param_key_value = (const struct param_journal_key_value_s*)(journal_entry->data);
        if (!memcmp(key, &stored_param_key_value->key, sizeof(struct param_key_s))) {
            return journal_entry;
        }
    }
    return NULL;
}

static void param_load_cache_value_from_all(uint16_t param_idx) {
    param_load_cache_value_from_hard_coded_default(param_idx);
    param_load_cache_value_from_journal(param_idx);
}

void param_acquire(void) {
    chMtxLock(&param_mutex);
}

void param_release(void) {
    chMtxUnlock(&param_mutex);
}

static uint8_t param_get_data_max_size(uint16_t param_idx) {
    if (param_idx >= num_params_registered) {
        return 0;
    }

    const struct param_descriptor_header_s* descriptor = param_descriptor_table[param_idx];

    switch(descriptor->type) {
        case PARAM_TYPE_FLOAT32:
            return sizeof(float);
        case PARAM_TYPE_INT64:
            return sizeof(uint64_t);
        case PARAM_TYPE_UINT32:
        case PARAM_TYPE_INT32:
            return sizeof(uint32_t);
        case PARAM_TYPE_UINT16:
        case PARAM_TYPE_INT16:
            return sizeof(uint16_t);
        case PARAM_TYPE_UINT8:
        case PARAM_TYPE_INT8:
        case PARAM_TYPE_BOOL:
            return sizeof(uint8_t);
        case PARAM_TYPE_STRING:
            return ((const struct param_descriptor_string_s*)descriptor)->max_len;
    }
    return 0;
}

static bool param_type_get_size_fixed(enum param_type_t type) {
    switch(type) {
        case PARAM_TYPE_UINT32:
        case PARAM_TYPE_UINT16:
        case PARAM_TYPE_UINT8:
        case PARAM_TYPE_INT64:
        case PARAM_TYPE_INT32:
        case PARAM_TYPE_INT16:
        case PARAM_TYPE_INT8:
        case PARAM_TYPE_STRING:
            return false;
        default:
            return true;
    }
}

static const struct param_journal_header_s* param_get_journal_header_ptr(struct flash_journal_instance_s* journal) {
    const struct flash_journal_entry_s* ret = NULL;

    if (!journal || !flash_journal_iterate(journal, &ret) || ret->len != sizeof(struct param_journal_header_s)) {
        return NULL;
    }

    return (const struct param_journal_header_s*)(ret->data);
}

static bool param_journal_iterate_final_values(const struct flash_journal_entry_s** iterator) {
    if (*iterator == NULL) {
        flash_journal_iterate(active_journal, iterator); // skip header
    }

    while (flash_journal_iterate(active_journal, iterator)) {
        const struct flash_journal_entry_s* journal_entry = *iterator;

        if (journal_entry->len < sizeof(struct param_journal_key_value_s)) {
            // journal_entry is too small to be a param entry
            continue;
        }

        // ensure this is the last occurrence of this key
        bool last_occurrence = true;
        const struct flash_journal_entry_s* later_journal_entry = journal_entry;
        while (flash_journal_iterate(active_journal, &later_journal_entry)) {
            if (later_journal_entry->len < sizeof(struct param_journal_key_value_s)) {
                // later_journal_entry is too small to be a param entry
                continue;
            }

            const struct param_journal_key_value_s* param_key_value = (const struct param_journal_key_value_s*)(journal_entry->data);
            const struct param_journal_key_value_s* later_param_key_value = (const struct param_journal_key_value_s*)(later_journal_entry->data);

            if (!memcmp(&param_key_value->key, &later_param_key_value->key, sizeof(struct param_key_s))) {
                last_occurrence = false;
                break;
            }
        }

        if (!last_occurrence) {
            continue;
        }

        return true;
    }
    return false;
}

static void param_generate_key(uint8_t name_len, const char* name, const enum param_type_t param_type, struct param_key_s* ret) {
    uint64_t hash;
    const uint8_t param_type_uint8 = (uint8_t)param_type;
    hash_fnv_1a(name_len, (uint8_t*)name, &hash);
    hash_fnv_1a(sizeof(param_type_uint8), &param_type_uint8, &hash);

    // xor-folding per http://www.isthe.com/chongo/tech/comp/fnv/
    hash = (hash>>40) ^ (hash&(((uint64_t)1<<40)-1));

    // write it to ret
    for (uint8_t i=0; i<5; i++) {
        ret->bytes[i] = (hash>>(8*i))&0xff;
    }
}

static int64_t param_uncompress_varint64(uint8_t len, const uint8_t* buf) {
    if (len == 0) {
        return 0;
    }

    int64_t ret = 0;

    for (uint8_t i=0; i<sizeof(uint64_t); i++) {
        if (i < len) {
            ret |= (uint64_t)buf[i]<<(8*i);
        } else if ((buf[len-1] & 0x80) == 0) {
            // sign bit = 0
            ret |= (uint64_t)0x00<<(8*i);
        } else {
            // sign bit = 1
            ret |= (uint64_t)0xFF<<(8*i);
        }
    }
    return ret;
}

static uint8_t param_compress_varint64(int64_t value, uint8_t* buf) {
    uint8_t len;
    for (len=0; len<sizeof(uint64_t); len++) {
        if (param_uncompress_varint64(len, buf) == value) {
            break;
        }
        buf[len] = (value>>(len*8))&0xFF;
    }
    return len;
}

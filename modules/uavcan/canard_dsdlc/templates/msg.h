@{from canard_dsdlc_helpers import *}@
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <modules/uavcan/uavcan.h>

@{
dep_headers = set()
for field in msg_fields:
    if field.type.category == field.type.CATEGORY_COMPOUND:
        dep_headers.add(msg_header_name(field.type))
    if field.type.category == field.type.CATEGORY_ARRAY and field.type.value_type.category == field.type.value_type.CATEGORY_COMPOUND:
        dep_headers.add(msg_header_name(field.type.value_type))
}@
@[  for header in dep_headers]@
#include <@(header)>
@[  end for]@

#define @(msg_underscored_name.upper())_MAX_PACK_SIZE @((msg_max_bitlen+7)/8)
#define @(msg_underscored_name.upper())_DT_SIG @('0x%08X' % (msg_dt_sig,))
@[  if msg_default_dtid is not None]@
#define @(msg_underscored_name.upper())_DT_ID @(msg_default_dtid)
@[  end if]@
@[  if msg_constants]
@[    for constant in msg_constants]@
#define @(msg_underscored_name.upper())_@(constant.name.upper()) @(constant.value)
@[    end for]@
@[  end if]@
@[  if msg_union]
enum @(msg_underscored_name)_type_t {
@[    for field in msg_fields]@
    @(msg_underscored_name.upper())_TYPE_@(field.name.upper()),
@[    end for]@
};
@[  end if]@

@(msg_c_type) {
@[  if msg_union]@
    enum @(msg_underscored_name)_type_t @(msg_underscored_name)_type;
    union {
@[    for field in msg_fields]@
@[      if field.type.category != field.type.CATEGORY_VOID]@
        @(field_cdef(field));
@[      end if]@
@[    end for]@
    };
@[  else]@
@[    for field in msg_fields]@
@[      if field.type.category != field.type.CATEGORY_VOID]@
    @(field_cdef(field));
@[      end if]@
@[    end for]@
@[  end if]@
};

@[  if msg_default_dtid is not None]@
extern const struct uavcan_message_descriptor_s @(msg_underscored_name)_descriptor;
@[  end if]@

void encode_@(msg_underscored_name)(@(msg_c_type)* msg, uavcan_serializer_chunk_cb_ptr_t chunk_cb, void* ctx);
uint32_t decode_@(msg_underscored_name)(const CanardRxTransfer* transfer, @(msg_c_type)* msg);
void _encode_@(msg_underscored_name)(uint8_t* buffer, @(msg_c_type)* msg, uavcan_serializer_chunk_cb_ptr_t chunk_cb, void* ctx, bool tao);
void _decode_@(msg_underscored_name)(const CanardRxTransfer* transfer, uint32_t* bit_ofs, @(msg_c_type)* msg, bool tao);

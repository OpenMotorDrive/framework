@[if msg.kind == msg.KIND_SERVICE]@
@{from canard_dsdlc_helpers import *}@
#pragma once
#include <@(msg_header_name_request(msg))>
#include <@(msg_header_name_response(msg))>

#define @(underscored_name(msg).upper())_DT_ID @(msg.default_dtid)
#define @(underscored_name(msg).upper())_DT_SIG @('0x%08X' % (msg.get_data_type_signature(),))
@[end if]@

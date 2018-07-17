@{from ubx_pdf_csv_parser_helper import *}
#include <@(msg_header_name(msg))>
#include <stddef.h>

@[if len(msg.ParsedStructFields)]@
@(msg_c_type(msg))* ubx_parse_@(msg_full_name(msg).lower())(const uint8_t* buffer, uint8_t buflen)
{
    if (buflen < sizeof(@(msg_c_type(msg)))) {
        return NULL;
    }
    return (@(msg_c_type(msg))*)buffer;
}
@[end if]@

@[if len(msg.ParsedStructRepFields)]@
@(msg_c_type_rep(msg))* ubx_parse_@(msg_full_name(msg).lower())_rep(const uint8_t* buffer, uint8_t buflen, uint8_t *num_repeat_blocks)
{
    uint8_t main_size = 0;
@[if len(msg.ParsedStructFields)]@
    @(msg_c_type(msg))* main_struct = ubx_parse_@(msg_full_name(msg).lower())(buffer, buflen);
    if (main_struct == NULL) {
        return NULL;
    }
    main_size = sizeof(@(msg_c_type(msg)));
@[  end if]@
@[  if len(msg.RepeatVarName) > 2]@
    *num_repeat_blocks = main_struct->@(msg.RepeatVarName);
    if (buflen < main_size + sizeof(@(msg_c_type_rep(msg)))*(*num_repeat_blocks)) {
        return NULL;
    }
@[  else]@
    *num_repeat_blocks = (buflen - main_size) / sizeof(@(msg_c_type_rep(msg)));
    if (*num_repeat_blocks == 0) {
        return NULL;
    }
@[  end if]@

    return (@(msg_c_type_rep(msg))*)(buffer + main_size);
}
@[end if]@

@[if len(msg.ParsedStructOptFields)]@
@(msg_c_type_opt(msg))* ubx_parse_@(msg_full_name(msg).lower())_opt(const uint8_t* buffer, uint8_t buflen)
{
    @(msg_c_type(msg))* main_struct = ubx_parse_@(msg_full_name(msg).lower())(buffer, buflen);
    if (main_struct == NULL) {
        return NULL;
    }
@[  if len(msg.ParsedStructRepFields)]@
    uint8_t num_repeat_blocks = 0;
    ubx_parse_@(msg_full_name(msg).lower())_rep(buffer, buflen, &num_repeat_blocks);
    if (buflen < sizeof(@(msg_c_type(msg))) + sizeof(@(msg_c_type_rep(msg)))*(num_repeat_blocks) + sizeof(@(msg_c_type_opt(msg)))) {
        return NULL;
    }
    return (@(msg_c_type_opt(msg))*)(buffer + sizeof(@(msg_c_type(msg))) + sizeof(@(msg_c_type_rep(msg)))*(num_repeat_blocks));
@[  else]@
    if (buflen < sizeof(@(msg_c_type(msg))) + sizeof(@(msg_c_type_opt(msg)))) {
        return NULL;
    }
    return (@(msg_c_type_opt(msg))*)(buffer + sizeof(@(msg_c_type(msg))));
@[  end if]@
}
@[end if]@

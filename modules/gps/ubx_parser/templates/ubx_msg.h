@{from ubx_pdf_csv_parser_helper import *}@
/*@(msg.TableStr)*/
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifndef @(msg_full_name_without_type(msg).upper())_CLASS_ID 
#define @(msg_full_name_without_type(msg).upper())_CLASS_ID @(msg.MsgClassId)
#endif
#ifndef @(msg_full_name_without_type(msg).upper())_MSG_ID
#define @(msg_full_name_without_type(msg).upper())_MSG_ID @(msg.MsgId)
#endif


@[if len(msg.ParsedStructFields)]@
@(msg_c_type(msg)) {
@[    for field in msg.ParsedStructFields]@
@[      if field[2] == 1]@
    @("%s %s" % (field[0], field[1]));
@[      else]@
    @("%s %s[%d]" % (field[0], field[1], field[2]));
@[      end if]@
@[    end for]@
};
@[if msg.MsgType not in ['PollRequest', 'Input', 'Command', 'Set']]@
@(msg_c_type(msg))* ubx_parse_@(msg_full_name(msg).lower())(const uint8_t* buffer, uint8_t buflen);
@[end if]@
@[end if]@

@[if len(msg.ParsedStructRepFields)]@
@(msg_c_type_rep(msg)) {
@[    for field in msg.ParsedStructRepFields]@
@[      if field[2] == 1]@
    @("%s %s" % (field[0], field[1]));
@[      else]@
    @("%s %s[%d]" % (field[0], field[1], field[2]));
@[      end if]@
@[    end for]@
};
@[  if msg.MsgType not in ['PollRequest', 'Input', 'Command', 'Set']]@
@(msg_c_type_rep(msg))* ubx_parse_@(msg_full_name(msg).lower())_rep(const uint8_t* buffer, uint8_t buflen, uint8_t *num_repeat_blocks);
@[  end if]@
@[end if]

@[if len(msg.ParsedStructOptFields)]@
@(msg_c_type_opt(msg)) {
@[    for field in msg.ParsedStructOptFields]@
@[      if field[2] == 1]@
    @("%s %s" % (field[0], field[1]));
@[      else]@
    @("%s %s[%d]" % (field[0], field[1], field[2]));
@[      end if]@
@[    end for]@
};
@[  if msg.MsgType not in ['PollRequest', 'Input', 'Command', 'Set']]@
@(msg_c_type_opt(msg))* ubx_parse_@(msg_full_name(msg).lower())_opt(const uint8_t* buffer, uint8_t buflen);
@[  end if]@
@[end if]@
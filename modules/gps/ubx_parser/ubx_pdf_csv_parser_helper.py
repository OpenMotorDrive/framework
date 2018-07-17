import os
import errno
import em
import math
import copy


def msg_c_type(obj):
    return 'struct __attribute__((__packed__)) %s_s' % (msg_full_name(obj).lower())

def msg_c_type_rep(obj):
    return 'struct __attribute__((__packed__)) %s_rep_s' % (msg_full_name(obj).lower())

def msg_c_type_opt(obj):
    return 'struct __attribute__((__packed__)) %s_opt_s' % (msg_full_name(obj).lower())

def msg_header_name(obj):
    return '%s.h' % ('ubx_'+obj.MsgName.lower()+'_'+obj.MsgType.lower(),)

def msg_c_file_name(obj):
    return '%s.c' % ('ubx_'+obj.MsgName.lower()+'_'+obj.MsgType.lower(),)

def msg_full_name(obj):
    return 'ubx_'+obj.MsgName.lower()+'_'+obj.MsgType.lower()

def msg_full_name_without_type(obj):
    return 'ubx_'+obj.MsgName.lower()

# https://stackoverflow.com/questions/600268/mkdir-p-functionality-in-python
def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

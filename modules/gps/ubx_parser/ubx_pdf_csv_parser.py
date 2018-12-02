import glob
import pandas as pd
from natsort import natsorted
from StringIO import StringIO
from tabulate import tabulate
import re
import numpy as np
import em
import os
from ubx_pdf_csv_parser_helper import *
import argparse
import sys
import copy

parser = argparse.ArgumentParser()
parser.add_argument('build_dir', nargs=1)
parser.add_argument('--build', action='append')
args = parser.parse_args()

build_dir = args.build_dir[0]
buildlist = None

if args.build:
    buildlist = set(args.build)

mkdir_p(os.path.dirname(os.path.join(build_dir, 'include/ubx_msgs.h')))
common_msg_header = open(os.path.join(build_dir, 'include/ubx_msgs.h'), 'wb')
templates = [
    {'source_file': 'ubx_msg.h',
     'output_file': 'include/@(msg_header_name(msg))'},
    {'source_file': 'ubx_msg.c',
     'output_file': 'src/@(msg_c_file_name(msg))'},
]
templates_dir = os.path.dirname(sys.argv[0]) + '/templates'
for template in templates:
    with open(os.path.join(templates_dir, template['source_file']), 'rb') as f:
        template['source'] = f.read()

class UbxCsvParser:

    TableColumns = {
        "VarType":'"",Short,Type,"Size\r(Bytes)",Comment,Min/Max,Resolution\r\n',
        "GnssID":'gnssId,GNSS\r\n',
        "MsgClass":'Name,Class,Description\r\n',
        "MsgID":'Page,Mnemonic,Cls/ID,Length,Type,Description\r\n'
    }
    def __init__(self):
        self.Msgs = {}
        self.GnssId = {}
        self.ByteDefs = []
        self.ClassDict = {}
        self.VarType = {}
        self.LineCount = 0
        self.curr_msg = None
        self.parsed_msg = None
        self.parsed_cnt = {}
        self.curr_msg_type = ""
        self.curr_df = None
        self.new_msg = False

    def selectParser(self, line):
        for key, value in self.TableColumns.iteritems():
            if line == value:
                return key
        return ""

    def doParseGeneric(self, filename, parsetype):
        df = pd.read_csv(filename)
        if (parsetype == "VarType"):
            pass
            # print tabulate(df, headers='keys', tablefmt='psql')
        elif (parsetype == "GnssID"):
            self.GnssId = df.set_index('GNSS').to_dict()["gnssId"]
        elif (parsetype == "MsgClass"):
            class_details = df.set_index('Name').to_dict('index')
            self.ClassDict.update(class_details)
        elif (parsetype == "MsgID"):
            df = df.dropna()
            msg_details = df.set_index(['Mnemonic','Type']).to_dict(orient='index')
            for key, value in sorted(msg_details.iteritems()):
                old_key = key
                key = (re.sub('[\.]', '', key[0]), re.sub('[^A-Za-z0-9]+', '', key[1]))
                msg_details[key] = value
                if key != old_key:
                    del msg_details[old_key]
            self.Msgs.update(msg_details)
        return

    def doParseMsg(self, lines, curr_msg):
        struct_field = ([line for line in lines if 'Byte Offset,"Number' in line])
        if len(struct_field):
            # print "\n", curr_msg, self.Msgs[curr_msg]['Cls/ID']
            # print "Page:", int(self.Msgs[curr_msg]['Page']), 'Length:' , self.Msgs[curr_msg]['Length']
            struct_table = '\r\n'.join(lines[lines.index(struct_field[0]):])
            df = pd.read_csv(StringIO(struct_table))
            df = df.dropna(axis='columns', how='all')
            if not self.new_msg:
                self.curr_df = self.curr_df.append(df,ignore_index=True)
            else:
                if self.parsed_msg is not None:
                    self.Msgs[self.parsed_msg]['DataFrame'] = self.curr_df
                self.curr_df = df
                self.parsed_msg = curr_msg
                self.new_msg = False
        return

    def parseFile(self, filename):
        file = open(filename, 'r')
        lines = file.readlines()
        parsetype = self.selectParser(lines[0])
        if (parsetype != ""):
            self.doParseGeneric(filename, parsetype)
        else:
            msg_name = None
            type_line = None
            if 'Message,' in lines[0]:
                #find MsgName
                prev_str = ""
                for s in self.Msgs.keys():
                    if s[0] in lines[0] and len(s[0]) > len(prev_str):
                        msg_name = s[0]
                        prev_str = s[0]
                if msg_name == None:
                    raise Exception(lines[0], "Bad MsgName")
            if msg_name:
                self.new_msg = True
                self.curr_msg = None
                #find type
                type_line = [line for line in lines if "Type," in line]
                if type_line:
                    type_line = type_line[0].split(',')
                    msg_type = re.sub('[^A-Za-z0-9]+', '', type_line[type_line.index("Type") + 1])
                    self.curr_msg = [key for key in ubx.Msgs.keys() if key[0] == msg_name and msg_type in key[1]][0]
                    if self.curr_msg in self.parsed_cnt.keys():
                        self.parsed_cnt[self.curr_msg] += 1
                        initial_msg_key = self.curr_msg
                        self.curr_msg = (self.curr_msg[0] + str(self.parsed_cnt[self.curr_msg]), self.curr_msg[1])
                        self.Msgs[self.curr_msg] = copy.deepcopy(self.Msgs[initial_msg_key])
                    else:
                        self.parsed_cnt[self.curr_msg] = 0
            if self.curr_msg is not None:
                self.doParseMsg(lines, self.curr_msg)
            else:
                raise Exception(msg_name, type_line, "Bad TypeName")
        if self.parsed_msg is not None:
            self.Msgs[self.parsed_msg]['DataFrame'] = self.curr_df

class UbxStruct:

    #+---------+-----------------------------+-----------+------------------+-------------------+------------------+
    #| Short   | Type                        |      Size | Comment          | Min/Max           | Resolution       |
    #|         |                             |   (Bytes) |                  |                   |                  |
    #+---------+-----------------------------+-----------+------------------+-------------------+------------------|
    #| U1      | Unsigned Char               |         1 |                  | 0..255            | 1                |
    #| RU1_3   | Unsigned Char               |         1 | binary floating  | 0..(31*2^7)       | ~ 2^(Value >> 5) |
    #|         |                             |           | point with 3 bit | non-continuous    |                  |
    #|         |                             |           | exponent, eeeb   |                   |                  |
    #|         |                             |           | bbbb, (Value &   |                   |                  |
    #|         |                             |           | 0x1F) << (Value  |                   |                  |
    #|         |                             |           | >> 5)            |                   |                  |
    #| I1      | Signed Char                 |         1 | 2's complement   | -128..127         | 1                |
    #| X1      | Bitfield                    |         1 |                  |                   |                  |
    #| U2      | Unsigned Short              |         2 |                  | 0..65535          | 1                |
    #| I2      | Signed Short                |         2 | 2's complement   | -32768..32767     | 1                |
    #| X2      | Bitfield                    |         2 |                  |                   |                  |
    #| U4      | Unsigned Long               |         4 |                  | 0..4'294'967'295  | 1                |
    #| I4      | Signed Long                 |         4 | 2's complement   | -2'147'483'648 .. | 1                |
    #|         |                             |           |                  | 2'147'483'647     |                  |
    #| X4      | Bitfield                    |         4 |                  |                   |                  |
    #| R4      | IEEE 754 Single Precision   |         4 |                  | -1*2^+127 ..      | ~ Value * 2^-24  |
    #|         |                             |           |                  | 2^+127            |                  |
    #| R8      | IEEE 754 Double Precision   |         8 |                  | -1*2^+1023 ..     | ~ Value * 2^-53  |
    #|         |                             |           |                  | 2^+1023           |                  |
    #| CH      | ASCII / ISO 8859.1 Encoding |         1 |                  |                   |                  |
    #+---------+-----------------------------+-----------+------------------+-------------------+------------------+
    UbxToCTypes = {
        'U1'    : 'uint8_t',
        'RU1_3' : 'uint8_t',      #currently unsupported by default in dsdl
        'I1'    : 'int8_t',
        'X1'    : 'uint8_t',
        'U2'    : 'uint16_t',
        'I2'    : 'int16_t',
        'X2'    : 'uint16_t',
        'U4'    : 'uint32_t',
        'I4'    : 'int32_t',
        'X4'    : 'uint32_t',
        'R4'    : 'float',
        'R8'    : 'double',
        'CH'    : 'int8_t'
    }
    def __init__(self, msginfo, msg):
        self.DataFrame = msginfo['DataFrame']
        self.MsgName = msg[0].replace('-', '_')
        self.MsgType = msg[1]
        self.MsgSize = msginfo['Length']
        self.MsgClassId = msginfo['Cls/ID'].split(' ')[0]
        self.MsgId = msginfo['Cls/ID'].split(' ')[1]

        self.TableStr =  "\nMsg: " + self.MsgName
        self.TableStr += "\nMsgType: " + self.MsgType
        self.TableStr += "\nMsgSize: " + self.MsgSize
        self.TableStr += "\nMsgClassId: " + self.MsgClassId
        self.TableStr += "\nMsgId: " + self.MsgId + "\n"
        self.TableStr += tabulate(self.DataFrame.set_index('Name'), headers='keys', tablefmt='psql')
        self.TableStr += "\n"
        # print "\n# ".join(self.TableStr.splitlines())

        self.RepeatedBlock = []
        self.RepeatVarName = ""
        self.extractRepeatedBlock() #removes the field that are part of repeated block
        self.OptionalBlock = []
        self.extractOptionalBlock() #removes the field that are part of optional block
        
        self.ParsedStructFields = []
        self.ParsedStructRepFields = []
        self.ParsedStructOptFields = []
        self.parseStructFields()
        # self.VarList = self.extractVarList()
        self.build_message()

    def extractRepeatedBlock(self):
        repeat_blocks_started = False
        for i, row in self.DataFrame.iterrows():
            m = None
            for key, value in row.iteritems():
                if type(value) is not str:
                    continue
                m = re.search('Start of repeated block \((.+?) times\)', value)
                if "End of repeated block" in value:
                    repeat_blocks_started = False
                    self.DataFrame.drop(i, inplace=True)
                    return
            if m:
                self.RepeatVarName = m.group(1)
                repeat_blocks_started = True
                self.DataFrame.drop(i, inplace=True)
                continue
            if repeat_blocks_started:
                self.RepeatedBlock.append(row)
                self.DataFrame.drop(i, inplace=True)

    def extractOptionalBlock(self):
        opt_blocks_started = False
        for i, row in self.DataFrame.iterrows():
            for key, value in row.iteritems():
                continue_i = False
                if type(value) is not str:
                    continue
                if 'Start of optional block' in value:
                    opt_blocks_started = True
                    self.DataFrame.drop(i, inplace=True)
                    continue_i = True
                    break
                if "End of optional block" in value:
                    opt_blocks_started = False
                    self.DataFrame.drop(i, inplace=True)
                    return
            if continue_i:
                continue
            if opt_blocks_started:
                self.OptionalBlock.append(row)
                self.DataFrame.drop(i, inplace=True)

    def parseStructFields(self):
        #Extract general vars into struct string
        for i,row in self.DataFrame.iterrows():
            if self.getParsedField(row):
                self.ParsedStructFields.append(self.getParsedField(row))

        #Extract repeated vars into struct string
        if len(self.RepeatedBlock):
            for row in self.RepeatedBlock:
                if self.getParsedField(row):
                    self.ParsedStructRepFields.append(self.getParsedField(row))

        if len(self.OptionalBlock):
            for row in self.OptionalBlock:
                if self.getParsedField(row):
                    self.ParsedStructOptFields.append(self.getParsedField(row))
        # print self.StructString,"\n"
        # print self.StructRepString,"\n"
        # print self.StructOptString,"\n"

    def getParsedField(self, row):
        fmts = [value for col, value in row.iteritems() if 'Number' in col]
        num_fmt = [value for value in fmts if value is not np.nan]
        if len(num_fmt):
            cfmt = [self.UbxToCTypes[key] for key in self.UbxToCTypes.keys() if key in num_fmt[0]]
            if not len(cfmt):
                return None
            carrsize = ""
            m  = re.search(r'\[(.*)\]', num_fmt[0])
            if m:
                carrsize = m.group(1)
            if cfmt:
                if len(carrsize):
                    return [cfmt[0], row['Name'], int(carrsize)]
                else:
                    return [cfmt[0], row['Name'], 1]
        return None

    def build_message(self):
        print 'building %s' % (self.MsgName,)
        global common_msg_header
        for template in templates:
            if template['source_file'] == 'ubx_msg.c' and (self.MsgType in ['PollRequest', 'Input', 'Command', 'Set']):
                continue
            output = em.expand(template['source'], msg=self)
            if not output.strip():
                continue

            output_file = os.path.join(build_dir, em.expand('@{from ubx_pdf_csv_parser_helper import *}'+template['output_file'], msg=self))
            mkdir_p(os.path.dirname(output_file))
            with open(output_file, 'wb') as f:
                f.write(output)
                if template['source_file'] == 'ubx_msg.h':
                    common_msg_header.write('#include <%s>\r\n'%output_file)

    def checkMsgSanity(self):
        if len(self.RepeatedBlock) and self.RepeatVarName:
            pass
        elif len(self.RepeatedBlock) or self.RepeatVarName:
            raise Exception("Bad Repeat Block")

ubx = UbxCsvParser()
f = natsorted(glob.glob(os.path.dirname(sys.argv[0]) + "/ubx_csv_tables/*.csv"))
for filename in f:
    # print filename
    ubx.parseFile(filename)

for key in sorted(ubx.Msgs.iterkeys()):
    if ubx.Msgs[key]['Length'] != "0":
        if 'DataFrame' in ubx.Msgs[key].keys():
            for i, row in ubx.Msgs[key]['DataFrame'].iterrows():
                if not pd.isnull(ubx.Msgs[key]['DataFrame'].at[i,'Name']):
                    ubx.Msgs[key]['DataFrame'].at[i,'Name'] = re.sub('[^A-Za-z0-9]+', '', ubx.Msgs[key]['DataFrame'].at[i,'Name'])
            if key[0] in buildlist:
                ubxstruct = UbxStruct(ubx.Msgs[key], key)

# print "GNSS ID"
# for key, value in ubx.GnssId.iteritems():
#     print key, value
# print "Class List"
# for key, value in ubx.ClassDict.iteritems():
#     print key, value
# print "Messages"
# for key, value in sorted(ubx.Msgs.iteritems()):
#     print key[0], '\t\t', key[1], '\t\t\t', value
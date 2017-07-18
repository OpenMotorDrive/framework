import crcmod.predefined
import sys
import os
import binascii
import struct

app_descriptor_fmt = "<8cQI"
SHARED_APP_DESCRIPTOR_SIGNATURES = ["\xd7\xe4\xf7\xba\xd0\x0f\x9b\xee", "\x40\xa2\xe4\xf1\x64\x68\x91\x06"]

crc64 = crcmod.predefined.Crc('crc-64-we')

with open(sys.argv[1], 'rb') as f:
    data = f.read()

app_descriptor_idx = None
app_descriptor_len = struct.calcsize(app_descriptor_fmt)
for i in range(0, len(data), 8):
    if data[i:i+8] in SHARED_APP_DESCRIPTOR_SIGNATURES:
        app_descriptor_idx = i
        break

app_descriptor = data[app_descriptor_idx:app_descriptor_idx+app_descriptor_len]

fields = list(struct.unpack(app_descriptor_fmt, app_descriptor))

fields[9] = len(data)

app_descriptor = struct.pack(app_descriptor_fmt, *fields)

data = data[:app_descriptor_idx] + app_descriptor + data[app_descriptor_idx+app_descriptor_len:]

crc64.update(data)

fields[8] = crc64.crcValue

app_descriptor = struct.pack(app_descriptor_fmt, *fields)

data = data[:app_descriptor_idx] + app_descriptor + data[app_descriptor_idx+app_descriptor_len:]

app_descriptor = data[app_descriptor_idx:app_descriptor_idx+app_descriptor_len]

with open(sys.argv[2], 'wb') as f:
    f.write(data)

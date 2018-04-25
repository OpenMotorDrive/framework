import uavcan
import sys
import os
import argparse

def get_firmware_crc_provided(data):
    import struct
    app_descriptor_fmt = "<8cQI"
    SHARED_APP_DESCRIPTOR_SIGNATURES = [b"\xd7\xe4\xf7\xba\xd0\x0f\x9b\xee", b"\x40\xa2\xe4\xf1\x64\x68\x91\x06"]

    app_descriptor_idx = None
    app_descriptor_len = struct.calcsize(app_descriptor_fmt)
    for i in range(0, len(data), 8):
        if data[i:i+8] in SHARED_APP_DESCRIPTOR_SIGNATURES:
            app_descriptor_idx = i
            break

    if app_descriptor_idx is None:
        return None

    app_descriptor = data[app_descriptor_idx:app_descriptor_idx+app_descriptor_len]

    fields = list(struct.unpack(app_descriptor_fmt, app_descriptor))

    return fields[8]

parser = argparse.ArgumentParser()
parser.add_argument('node_name', nargs=1)
parser.add_argument('firmware_name', nargs=1)
parser.add_argument('bus', nargs=1)
parser.add_argument('--discovery_time', nargs=1, default=5)
args = parser.parse_args()

with open(args.firmware_name[0], 'rb') as f:
    firmware_crc = get_firmware_crc_provided(f.read())

print('%X' % (firmware_crc,))
update_complete = {}

def monitor_update_handler(e):
    global update_complete
    if e.event_id == node_monitor.UpdateEvent.EVENT_ID_INFO_UPDATE:
        print(e.entry)
        if e.entry.info.name == args.node_name[0]:
            if e.entry.info.software_version.image_crc != firmware_crc:
                if e.entry.status.mode != e.entry.status.MODE_SOFTWARE_UPDATE:
                    print('updating %u' % (e.entry.node_id,))
                    req_msg = uavcan.protocol.file.BeginFirmwareUpdate.Request(source_node_id=node.node_id, image_file_remote_path=uavcan.protocol.file.Path(path=args.firmware_name[0]))
                    node.request(req_msg, e.entry.node_id, update_response_handler)
                update_complete[e.entry.node_id] = False
            else:
                print('%u up to date' % (e.entry.node_id,))
                update_complete[e.entry.node_id] = True


def update_response_handler(e):
    assert e.response.error == e.response.ERROR_OK

node = uavcan.make_node(args.bus[0])
node.node_id = 126

file_server = uavcan.app.file_server.FileServer(node, [args.firmware_name[0]])
node_monitor = uavcan.app.node_monitor.NodeMonitor(node)
allocator = uavcan.app.dynamic_node_id.CentralizedServer(node, node_monitor)
node_monitor.add_update_handler(monitor_update_handler)

# discover nodes
node.spin(int(args.discovery_time[0]))

# wait for updates to complete
while False in update_complete.values():
    node.spin(1)

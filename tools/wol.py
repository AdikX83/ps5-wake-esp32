#!/usr/bin/env python3
"""Send a Wake-on-LAN magic packet (UDP broadcast) - handy for testing PS5 Wake.

    python tools/wol.py DC:1E:D5:00:00:00                 # MAC of the ESP32 (or of the PS5, if set in the panel)
    python tools/wol.py DC:1E:D5:00:00:00 --broadcast 192.168.1.255 --port 9
"""
import argparse
import re
import socket

parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("mac", help="target MAC address, e.g. AA:BB:CC:DD:EE:FF")
parser.add_argument("--broadcast", default="255.255.255.255", help="broadcast address (default 255.255.255.255)")
parser.add_argument("--port", type=int, default=9, help="UDP port, 9 (default) or 7")
args = parser.parse_args()
if not re.fullmatch(r"(?:[0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}", args.mac):
    parser.error("MAC must look like AA:BB:CC:DD:EE:FF")
packet = b"\xff" * 6 + bytes.fromhex(re.sub("[:-]", "", args.mac)) * 16
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.sendto(packet, (args.broadcast, args.port))
print(f"Magic packet for {args.mac.upper()} sent to {args.broadcast}:{args.port}")

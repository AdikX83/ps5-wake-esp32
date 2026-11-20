#!/usr/bin/env python3
"""Fail if tracked files contain data that must not be published: private IPv4 addresses, MAC/Bluetooth
addresses that are not documented examples, Wi-Fi credentials or tokens. Run before every release (CI does)."""
import re
import subprocess
import sys

ALLOWED_MACS = {  # examples and test vectors used in docs and tests
    "AA:BB:CC:DD:EE:FF", "00:11:22:33:44:55", "11:22:33:44:55:66", "00:00:00:00:00:00", "FF:FF:FF:FF:FF:FF",
    "DC:1E:D5:11:22:33", "DC:1E:D5:00:00:00", "AB:12:34:56:78:90",
}
MAC = re.compile(r"\b(?:[0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}\b")
PRIVATE_IP = re.compile(r"\b(?:10\.\d{1,3}|172\.(?:1[6-9]|2\d|3[01])|192\.168)\.\d{1,3}\.\d{1,3}\b")
ALLOWED_IPS = {"192.168.4.1", "192.168.4.2", "192.168.1.255", "192.168.0.255", "192.168.1.10", "192.168.1.50"}
SECRET = re.compile(r"(?i)(gh[pousr]_[A-Za-z0-9]{20,}|BEGIN (?:RSA |EC )?PRIVATE KEY|password\s*[:=]\s*['\"][^'\"]{4,})")
SKIP = ("third_party/", ".gitmodules")

files = subprocess.check_output(["git", "ls-files", "--cached", "--others", "--exclude-standard"], text=True).split()
problems = []
for name in files:
    if name.startswith(SKIP):
        continue
    try:
        text = open(name, encoding="utf-8").read()
    except (UnicodeDecodeError, IsADirectoryError, FileNotFoundError):
        continue
    for number, line in enumerate(text.splitlines(), 1):
        for mac in MAC.findall(line):
            if mac.upper().replace("-", ":") not in ALLOWED_MACS and not mac.upper().startswith("FF:FF:FF"):
                problems.append(f"{name}:{number}: MAC/BT address {mac}")
        for ip in PRIVATE_IP.findall(line):
            if ip not in ALLOWED_IPS:
                problems.append(f"{name}:{number}: private IP {ip}")
        if SECRET.search(line):
            problems.append(f"{name}:{number}: possible secret")
if problems:
    print("\n".join(problems))
    sys.exit(f"privacy scan: {len(problems)} problem(s)")
print(f"privacy scan: {len(files)} files clean")

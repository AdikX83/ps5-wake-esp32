#!/usr/bin/env python3
"""PS5 Wake - DualSense address reader for Linux and macOS (also works on Windows with hidapi).

Reads, over USB and read-only, the Bluetooth address of the DualSense and of the PS5 it is paired
with (HID feature report 0x09), and prints a setup code for the PS5 Wake panel.

Linux: no extra packages; run with sudo if access to /dev/hidraw* is denied.
macOS / Windows: pip3 install hidapi
"""
import glob
import os
import subprocess
import sys
import time

SONY = 0x054C
PRODUCTS = (0x0CE6, 0x0DF2)  # DualSense, DualSense Edge


def mac(report, offset):
    return ":".join(f"{b:02X}" for b in reversed(report[offset:offset + 6]))


def read_linux():
    import fcntl
    for node in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        try:
            with open(os.path.join(node, "device", "uevent"), encoding="ascii", errors="ignore") as f:
                uevent = f.read().upper()
        except OSError:
            continue
        if not any(f"HID_ID=0003:{SONY:08X}:{p:08X}" in uevent for p in PRODUCTS):
            continue
        buffer = bytearray(20)
        buffer[0] = 0x09
        request = (3 << 30) | (len(buffer) << 16) | (ord("H") << 8) | 0x07  # HIDIOCGFEATURE(20)
        try:
            fd = os.open("/dev/" + os.path.basename(node), os.O_RDWR)
        except PermissionError:
            sys.exit("Permission denied for /dev/hidraw*. Run again with sudo.")
        try:
            fcntl.ioctl(fd, request, buffer, True)
        except OSError:
            continue
        finally:
            os.close(fd)
        if buffer[0] == 0x09:
            return bytes(buffer)
    return None


def read_hidapi():
    try:
        import hid
    except ImportError:
        sys.exit("The hidapi module is missing. Install it with:  pip3 install hidapi")
    for info in hid.enumerate(SONY, 0):
        if info["product_id"] not in PRODUCTS:
            continue
        device = hid.device()
        try:
            device.open_path(info["path"])
            report = bytes(device.get_feature_report(0x09, 20))
        except (OSError, IOError):
            continue
        finally:
            device.close()
        if len(report) >= 16 and report[0] == 0x09:
            return report
    return None


def copy_to_clipboard(text):
    for command in (["pbcopy"], ["wl-copy"], ["xclip", "-selection", "clipboard"], ["clip"]):
        try:
            subprocess.run(command, input=text.encode(), check=True, capture_output=True)
            return True
        except (OSError, subprocess.CalledProcessError):
            continue
    return False


def main():
    print("PS5 Wake - DualSense address reader")
    print("Connect the DualSense (already paired with your PS5) to this computer with a USB cable.\n")
    reader = read_linux if sys.platform.startswith("linux") else read_hidapi
    report = None
    for attempt in range(60):
        report = reader()
        if report:
            break
        if attempt == 0:
            print("Waiting for the controller (up to 60 s)...")
        time.sleep(1)
    if not report:
        sys.exit("No DualSense found over USB. Check the cable (it must carry data) and try again.")
    ps5, pad = mac(report, 10), mac(report, 1)
    if ps5 == pad or ps5 == "00:00:00:00:00:00":
        sys.exit("The controller is not paired with a console. Pair it with your PS5 first.")
    code = f"PS5WAKE:{ps5},{pad}"
    print(f"PS5 Bluetooth address:        {ps5}")
    print(f"Controller Bluetooth address: {pad}\n")
    print(f"Setup code: {code}")
    if copy_to_clipboard(code):
        print("The setup code has been copied to the clipboard - paste it into the PS5 Wake panel.")
    else:
        print("Copy the setup code above and paste it into the PS5 Wake panel.")


if __name__ == "__main__":
    main()

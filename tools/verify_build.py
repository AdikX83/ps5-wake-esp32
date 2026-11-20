#!/usr/bin/env python3
"""Audit a firmware build and write build/verification.json.

Run after `idf.py build` from the firmware/ directory (ESP-IDF 5.5.5 environment active):
    python ../tools/verify_build.py
Checks the pinned Soft-BR submodule, the reviewed local patch, the ESP-IDF version, the configuration,
the linked radio symbols, the single library hook and the partition layout.
RESULT: PASS means build integrity only - it is not proof that the radio or a PS5 wake works.
"""
from __future__ import annotations

import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / "firmware"
BUILD = FIRMWARE / "build"
UPSTREAM = ROOT / "third_party" / "esp32-p4-c6-le-audio"
EXPECTED_UPSTREAM = "0fafdd428ea26f8bb700d67d72e2dcf12241f4c5"
EXPECTED_IDF = "v5.5.5"
UPSTREAM_RADIO_SHA256 = "340addd83500b81c0c804786f5127cfc0eae39119040736372eea90ea48cb145"
PATCHED_RADIO_SHA256 = "fb799d72dfedfee4273db38c64617aaa92067bea6fcfce94910269c2a94e9042"


def idf_version_from(sdk: Path) -> str:
    """Read the ESP-IDF version from tools/cmake/version.cmake (works in shallow clones and Docker images)."""
    text = (sdk / "tools" / "cmake" / "version.cmake").read_text(encoding="utf-8")
    parts = [re.search(rf"IDF_VERSION_{name}\s+(\d+)", text).group(1) for name in ("MAJOR", "MINOR", "PATCH")]
    return "v" + ".".join(parts)


def run(*args: str) -> str:
    return subprocess.check_output(args, text=True, encoding="utf-8").strip()


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    sdk = Path(os.environ["IDF_PATH"])
    require(run("git", "-C", str(UPSTREAM), "rev-parse", "HEAD") == EXPECTED_UPSTREAM, "Wrong Soft-BR revision")
    subprocess.run(["git", "-C", str(UPSTREAM), "diff", "--exit-code", "--quiet", "HEAD", "--",
                    "br_ll", "cis_ll", "c6_firmware/main", "p4_bt_classic/src"], check=True)
    idf_version = os.environ.get("IDF_VERSION") or idf_version_from(sdk)
    require(idf_version.startswith(EXPECTED_IDF), f"ESP-IDF {EXPECTED_IDF} required, found {idf_version}")
    radio_upstream = UPSTREAM / "c6_firmware/main/br_c6_radio.c"
    radio_patched = FIRMWARE / "softbr_patched/br_c6_radio.c"
    require(digest(radio_upstream) == UPSTREAM_RADIO_SHA256, "Unexpected upstream br_c6_radio.c")
    require(digest(radio_patched) == PATCHED_RADIO_SHA256,
            "softbr_patched/br_c6_radio.c differs from the reviewed patch (br_c6_radio.patch)")
    config = (FIRMWARE / "sdkconfig").read_text(encoding="utf-8").splitlines()
    for setting in ('CONFIG_IDF_TARGET="esp32c6"', "CONFIG_BT_CONTROLLER_ONLY=y",
                    "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y", "CONFIG_ESP_DEFAULT_TASK_PRIO=22",
                    "CONFIG_ESP_HTTPS_SERVER_ENABLE=y"):
        require(setting in config, f"Missing configuration: {setting}")
    elf = BUILD / "ps5_wake.elf"
    nm = "riscv32-esp-elf-nm"
    symbols = {line.split()[-1] for line in run(nm, "--defined-only", str(elf)).splitlines() if line.split()}
    required = {"br_c6_init", "br_c6_h2c", "br_c6_radio_init", "br_engine_hci_from_host", "wake_radio_attempt",
                "btc_lmp_start", "btc_lmp_on_pdu", "__wrap_esp_panic_handler", "__wrap_esp_netif_receive",
                "esp_panic_handler", "r_ble_ll_event_send", "r_ble_lll_sched_insert", "r_ble_phy_disable"}
    require(required <= symbols, f"Missing linked symbols: {required - symbols}")
    require("cis_engine_init" not in symbols, "Unexpected LE Audio engine in the firmware")
    partitions = (BUILD / "partition_table" / "partition-table.bin").read_bytes()
    factory = None
    for offset in range(0, len(partitions) - 31, 32):
        magic, kind, subtype, address, size, _label, _flags = struct.unpack_from("<HBBLL16sL", partitions, offset)
        if magic == 0x50AA and kind == 0 and subtype == 0:
            factory = (address, size)
            break
    require(factory is not None, "Factory app partition is missing")
    app = BUILD / "ps5_wake.bin"
    require(app.stat().st_size <= factory[1] and factory[0] + factory[1] <= 0x400000, "App does not fit in 4 MB")
    library = sdk / "components/bt/controller/lib_esp32c6/esp32c6-bt-lib/esp32c6/libble_app.a"
    subprocess.run([sys.executable, str(UPSTREAM / "c6_firmware/cmake/verify_hooks.py"),
                    "--elf", str(elf), "--map", str(BUILD / "ps5_wake.map"),
                    "--hooks", str(FIRMWARE / "cmake/ble_hooks.txt"), "--orig-lib", str(library)], check=True)
    manifest = {
        "result": "PASS", "meaning": "Build integrity only; not proof of RF behaviour or of a PS5 wake",
        "version": (ROOT / "VERSION").read_text(encoding="utf-8").strip(),
        "upstream_commit": EXPECTED_UPSTREAM, "idf_version": idf_version, "target": "esp32c6",
        "softbr_local_patch": {"file": "c6_firmware/main/br_c6_radio.c",
                               "upstream_sha256": UPSTREAM_RADIO_SHA256, "patched_sha256": PATCHED_RADIO_SHA256},
        "controller_library_sha256": digest(library), "verified_radio_hook": "r_ble_lll_sched_pop_executed_sch",
        "app_bytes": app.stat().st_size, "app_partition": {"offset": factory[0], "bytes": factory[1]},
        "artifacts": {name: {"bytes": (BUILD / name).stat().st_size, "sha256": digest(BUILD / name)}
                      for name in ("ps5_wake.bin", "bootloader/bootloader.bin", "partition_table/partition-table.bin")},
    }
    (BUILD / "verification.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"App: {app.stat().st_size} / {factory[1]} bytes; pinned Soft-BR, patch, ESP-IDF and radio symbols verified")
    print("RESULT: PASS")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, OSError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"RESULT: FAIL: {error}") from error

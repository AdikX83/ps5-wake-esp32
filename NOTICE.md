# Notices

## This project

ps5-wake-esp32 is licensed under the MIT License (see [LICENSE](LICENSE)).

## Third-party code

| Component | Where | License | Notes |
|---|---|---|---|
| **esp32-p4-c6-le-audio** (Soft-BR Bluetooth Classic engine and ESP32-C6 adapter) by Ernest Domaradzki | `third_party/esp32-p4-c6-le-audio` (git submodule, pinned to `0fafdd4`) | MIT | Built from the pinned commit. One file is built from a modified copy: `firmware/softbr_patched/br_c6_radio.c`, see `br_c6_radio.patch`. `firmware/cmake/ble_hooks.cmake` and `patch_ble_lib.cmake` are copies from the same repository (`firmware/cmake/LICENSE`). The LMP engine `p4_bt_classic/src/btc_lmp.cpp` is compiled unchanged. |
| **ESP-IDF 5.5.5** and its components (Espressif Systems and others) | not in this repository | Apache 2.0 and others, see the ESP-IDF COPYRIGHT file | Linked into the firmware. |
| **ESP32-C6 Bluetooth controller library** `libble_app.a` (Espressif Systems) | part of ESP-IDF | Apache 2.0 | **Modified at build time:** a copy is made in which the single symbol `r_ble_lll_sched_pop_executed_sch` is weakened so that the Soft-BR adapter can hook it (`firmware/cmake/ble_hooks.txt`). The original library in ESP-IDF is not changed. Firmware binaries published by this project contain this modified copy. |
| **espressif/mdns** (Espressif Systems) | downloaded by the ESP-IDF component manager | Apache 2.0 | `ps5-wake-xxxx.local` name for the setup panel. |
| **cJSON** (Dave Gamble and contributors) | part of ESP-IDF | MIT | |

## Prior art

The PS5 wake method (paging the console with the Bluetooth address of a paired controller) and the DualSense
USB feature report layout were studied in [bizkut/ps5-esp32-wakeup](https://github.com/bizkut/ps5-esp32-wakeup)
(Apache 2.0), which credits [FreeTHX/pywakepsXonbt](https://github.com/FreeTHX/pywakepsXonbt) (Apache 2.0).
The implementation in this repository is new.

## Trademarks

PlayStation, PS5 and DualSense are trademarks or registered trademarks of Sony Interactive Entertainment Inc.
ESP32 is a trademark of Espressif Systems. This project is unofficial and is not affiliated with, endorsed by or
sponsored by Sony Interactive Entertainment or Espressif Systems.

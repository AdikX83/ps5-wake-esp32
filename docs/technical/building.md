# Building, testing and releasing

## Requirements

- **ESP-IDF v5.5.5** exactly. The Soft-BR hook is applied to the LE controller library shipped with this version;
  `tools/verify_build.py` refuses other versions.
- Python 3.9+ and a C compiler (gcc or clang) for the host tests.
- An ESP32-C6 board for flashing.

## Clone

```sh
git clone --recursive https://github.com/AdikX83/ps5-wake-esp32.git
# or, in an existing clone:
git submodule update --init --recursive
```

`third_party/esp32-p4-c6-le-audio` must be at commit `0fafdd4`.

## Build and flash

```sh
cd firmware
idf.py set-target esp32c6
idf.py build
python ../tools/verify_build.py      # must print RESULT: PASS
idf.py -p PORT flash monitor          # PORT: COM3, /dev/ttyACM0, /dev/cu.usbmodem…
```

`verify_build.py` checks the ESP-IDF version, the submodule commit, the SHA-256 of the upstream and the patched
radio file, the configuration, the linked Soft-BR symbols, the library hook and the partition table, and writes
`build/verification.json`. A PASS means the build is the reviewed one – not that the radio works with your console.

`sdkconfig.defaults` is applied only when `firmware/sdkconfig` does not exist yet: after pulling changes to it, delete
`firmware/sdkconfig` (or run `idf.py fullclean`) before building. `verify_build.py` fails if the HTTPS server is not
enabled in the configuration.

`idf.py flash` keeps the settings stored in NVS. To start from scratch use `idf.py erase-flash` first, or hold BOOT
for 20 seconds after flashing.

### Docker

```sh
docker run --rm -v "$PWD":/project -w /project/firmware espressif/idf:v5.5.5 \
  sh -c 'idf.py set-target esp32c6 build && python ../tools/verify_build.py'
```

## Host tests

```sh
tests/run.sh
```

Compiles the Wake-on-LAN parser and the wake protocol code together with the pinned Soft-BR sources for the host,
with AddressSanitizer and UndefinedBehaviorSanitizer.

## Privacy check

```sh
python tools/privacy_scan.py
```

Fails if the repository contains anything that looks like a real MAC/Bluetooth address, a private IP address or a
secret. Documentation must use the example addresses listed in the script. Run it before every commit; CI runs it
on every push.

## Serial console

At 115200 baud the firmware logs its state; typing `STATUS` and Enter prints a one-line summary (version, Wi-Fi, IP,
last wake result, WoL counters). Opening the port with DTR/RTS asserted resets the board – most terminal programs
do this, which is harmless.

## Continuous integration

`.github/workflows/ci.yml` runs on every push and pull request:

1. host tests,
2. privacy scan,
3. firmware build in the `espressif/idf:v5.5.5` container and `verify_build.py`,
4. uploads the binaries as a workflow artifact.

## Releases

1. Update `VERSION` and `CHANGELOG.md`.
2. Commit, then tag: `git tag v1.2.3 && git push --tags`.
3. `.github/workflows/release.yml` builds the firmware, creates a GitHub release with the binaries
   (`bootloader.bin`, `partition-table.bin`, `ps5_wake.bin`, and a merged `ps5-wake-esp32c6-<version>.bin` for
   flashing at offset 0x0) and publishes the web installer to GitHub Pages.

### Web installer

`web/` contains the installer page using [ESP Web Tools](https://esphome.github.io/esp-web-tools/). The release
workflow copies the binaries next to it and writes two manifests:

- `manifest.json` (**Install or update**) flashes the bootloader at `0x0`, the partition table at `0x8000` and the
  application at `0x10000`. NVS (`0x9000`) is not touched, so the settings are kept.
- `manifest-clean.json` (**Clean install**) also writes `blank-settings.bin`, 28 KB of `0xFF`, at `0x9000`, which
  empties NVS and `phy_init` and so deletes the settings.

The firmware does not implement Improv Serial, so ESP Web Tools treats every installation as a new one and asks
whether to erase the whole chip (`new_install_prompt_erase`). The page replaces that question with a note and never
erases the whole chip: on the ESP32-C6's native USB port the browser's chip erase does run, but its confirmation does
not reach the page and the installer waits forever. Writing an empty settings area avoids the full erase. The page
also removes the fixed "This will take 2 minutes" estimate from the dialog.

## Flash layout

| Name | Offset | Size |
|---|---|---|
| bootloader | 0x0 | – |
| partition table | 0x8000 | 0x1000 |
| nvs | 0x9000 | 0x6000 |
| phy_init | 0xF000 | 0x1000 |
| factory (app) | 0x10000 | 0x300000 |

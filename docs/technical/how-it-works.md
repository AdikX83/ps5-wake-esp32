# How it works

This page describes the firmware for people who want to understand, review or extend it. For setup instructions
see the [user guide](../en/user-guide.md).

## Overview

```
 WoL sender ──UDP 9/7 or EtherType 0x0842──▶ ESP32-C6 (Wi-Fi STA)
                                               │  wake request (also: BOOT button, panel "Wake now")
                                               ▼
                                     Soft-BR (Bluetooth Classic in software
                                     on the ESP32-C6 BLE radio)
                                               │  page PS5 as the paired DualSense
                                               ▼
                                            PS5 powers on
```

The ESP32-C6 has no Bluetooth Classic (BR/EDR) controller – only Bluetooth LE. The PS5, however, only wakes up for
a **Bluetooth Classic** connection from a controller it is paired with. PS5 Wake bridges that gap with
**Soft-BR**, a software implementation of the BR baseband that drives the C6's LE radio directly.

## Wake-on-LAN reception (`wol.c`, `main.c`)

- UDP magic packets on ports **9** and **7** (broadcast or unicast) are parsed by `wol.c`: 6 × `0xFF` followed by
  16 repetitions of the target MAC (anywhere in the payload; extra bytes such as a SecureOn password are ignored).
- Some routers (e.g. Asuswrt-Merlin's `ether-wake`) send **raw Ethernet frames** with EtherType `0x0842`, which lwIP
  never sees. The firmware wraps `esp_netif_receive` (`-Wl,--wrap=esp_netif_receive`) to inspect incoming frames
  before they reach the IP stack and passes such frames to the same parser.
- A packet is accepted when its target MAC equals the ESP32's own Wi-Fi MAC or, if configured, the PS5's network MAC.
- Accepted requests are rate-limited: one attempt at a time and at least 30 s between attempts.

## Waking the console (`wake_radio.c`, `wake_protocol.c`)

1. The ESP32 temporarily presents itself with the **DualSense's Bluetooth address**.
2. Soft-BR pages the **PS5's Bluetooth address** (ID packets with the console's access code, frequency hopping as
   defined by the BR specification).
3. When the PS5 answers, the firmware runs the link manager (LMP) procedure used to set up a connection
   (`host_connection_req`, from the upstream `btc_lmp.cpp`).
4. A paired controller initiating this procedure is what the PS5 treats as "PS button pressed": it powers on.
   The link is then dropped – the real controller takes over normally.

The attempt result (console answered / no answer) is shown on the LED and in the panel status.

### Soft-BR and the local patch

Soft-BR comes from [ernimen7/esp32-p4-c6-le-audio](https://github.com/ernimen7/esp32-p4-c6-le-audio), pinned as a
git submodule at `0fafdd4`. The project uses a single, reviewed change, kept in
`firmware/softbr_patched/br_c6_radio.c` (diff in `br_c6_radio.patch`):

- The C6 radio receives BR ID packets through the LE packet engine, which interprets bits of the
  LAP (`LAP[13:6]`) as an LE length field. For the DualSense addresses tested that value is large (e.g. 224 bytes),
  so the receive window was cut short and the console's reply was lost. The patch uses a fixed RX overrun of
  `BR_RX_ID_OVERRUN_US` (2300 µs) for all slots.

`tools/verify_build.py` checks the submodule commit, the SHA-256 of the upstream and patched file, and the linked
symbols, so a build cannot silently use a different radio implementation.

Soft-BR needs one scheduler hook inside Espressif's LE controller library. `firmware/cmake/patch_ble_lib.cmake`
copies `libble_app.a` from ESP-IDF into the build directory and applies that hook to the copy (ESP-IDF itself is not
modified). The library is Apache-2.0 licensed; see [NOTICE.md](../../NOTICE.md).

## Console state (`ps5_state.c`)

Every 10 seconds (every 2 seconds for a minute after a wake attempt) the ESP32 broadcasts a PlayStation Device
Discovery Protocol request (`SRCH * HTTP/1.1`, `device-discovery-protocol-version:00030010`) to UDP port 9302. A PS5
answers `200 Ok` when it is on and `620 Server Standby` in rest mode (only if it stays connected to the network in
rest mode). Each poll sends the request twice (UDP gets lost); a change between on and standby needs the same answer twice in a
row (in rest mode the console sometimes answers "on" for a moment); no answer for 60 seconds means off or not
reachable. Measured: switched off from the menu the console goes silent at once; going to rest mode it either answers
"standby" at once or is silent for a while first. With the PS5's network MAC set, only replies
whose `host-id` matches count. The state is shown in the panel (dot on the power icon), in `/api/status` and over
MQTT.

## Several devices in one network

Every device has its own mDNS name: the device name from the panel (lower case, accents removed, spaces as dashes,
e.g. "Pokój Kuby" → `pokoj-kuby.local`) or, without a name, `ps5-wake-` and the same 4 hex digits as its panel
network `PS5-WAKE-XXXX`. Choose a name that no other device in the network uses. The HTTPS certificate is created again when the name changes. Home Assistant shows the device under its
name; MQTT topics and entity IDs use the MAC address and never collide. The panel lists the consoles
that answer discovery; choosing one stores its network MAC, so the console state follows that console only.

## Wi-Fi

- Station mode, 2.4 GHz only, WPA2/WPA3-Personal. Country code is configurable (channel range).
- After a disconnect the firmware reconnects with a short delay (WPA3 access points may reject an immediate
  reconnect with reason 209). If the ESP32 stays offline for 2 minutes it restarts.

## Setup panel (`portal.c`, `portal.html`)

- Access point `PS5-WAKE-XXXX` (last two bytes of the MAC), WPA2, default password `ps5wol-setup`. Saving is refused
  until the user sets an own panel password.
- **HTTPS everywhere.** `esp_https_server` on port 443 serves the panel with a self-signed EC P-256 certificate
  generated on the device (`panel_tls.c`, subject alternative names `ps5-wake-xxxx.local` and `192.168.4.1`) and stored in
  NVS. A plain HTTP server on port 80 only redirects to HTTPS. Browsers warn on the first visit because no public
  authority signs the certificate; the panel shows its SHA-256 fingerprint and expiry, offers it for download (to
  trust it in the system) and can create a new one ("New certificate"). At most 2 TLS sessions are open at a time.
- **Certificate dates.** When the device is on the home network it gets the time over NTP (`pool.ntp.org`); the
  certificate is valid for 825 days (the limit Apple platforms apply to TLS certificates) and is renewed
  automatically 30 days before it expires (checked every hour). Without a clock (a new device in the panel network)
  the 825 days are counted from the firmware build date, and the certificate is replaced as soon as NTP gives the
  time.
- **Panel network** (`192.168.4.1`, mDNS `ps5-wake-xxxx.local`): always HTTPS. There is no captive portal – the page does
  not open by itself; the user opens `https://192.168.4.1`. Without "Access from home network", connections that do not
  arrive through the panel's access point (for example from the home network to the ESP32's station IP) are refused.
- **"Access from home network"** (on by default; when switched off the panel exists only in the panel network for 10
  minutes after holding BOOT) does not start the access point. After a restart the panel is served on the home network
  only (the address from DHCP and `ps5-wake-xxxx.local`), and every request needs HTTP Basic authentication with the
  panel user name (default `admin`) and the panel password (wrong credentials are delayed by 1 s). HTTPS is on by default and can be
  switched off for the home network in the panel. Holding BOOT for 5 s still opens the access point as usual; it then
  closes after 10 minutes idle while the home-network panel stays available.
- The page is a single embedded file with all 30 languages; language switching happens without reloading.
- API: `GET /api/status`, `GET /api/scan`, `POST /api/save`, `POST /api/wake`, `POST /api/exit`, `POST /api/lang`,
  `POST /api/extend`, `POST /api/newcert`; downloads `/download/ps5-wake.crt`, `/download/ps5-wake-reader.cmd` and `/download/ps5_wake_reader.py`.
- An unconfigured device (new, or after a factory reset) starts the panel network by itself and keeps it on until
  the first save. Otherwise the access point closes (by restarting the ESP32) after saving, after "Close", or after 10 minutes
  idle.

## Settings storage

All settings are kept in NVS (namespace with keys for Wi-Fi, Bluetooth addresses, WoL MAC, country, language, LED and
panel options, panel password). A 20-second press of BOOT erases them. Flashing a new firmware version through the
web installer keeps NVS.

## Address reader (`tools/reader`)

The DualSense reports, over USB HID feature report `0x09`, its own Bluetooth address and the address of the console
it is paired with. The reader reads this report (Windows: built-in HID APIs via PowerShell; macOS/Linux: Python) and
prints a setup code `PS5WAKE:<ps5>:<controller>` for the panel. Nothing is sent anywhere.

## Limitations

- Only waking is supported – the ESP32 does not act as a controller and cannot turn the console off.
- Tested with ESP32-C6-DevKitM-1 and one PS5 console; other C6 boards should work but are untested.
- Range is similar to a controller's; keep the ESP32 within a few metres of the console.

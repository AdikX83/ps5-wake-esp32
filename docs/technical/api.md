# REST API

The setup panel is a single page that talks to the ESP32 through a small JSON API. The same API can be used by
scripts and home-automation systems.

## Where the API is available

| Access | Address | Authentication |
|---|---|---|
| Panel network `PS5-WAKE-xxxx` (BOOT 5 s, or a new device) | `https://192.168.4.1` | none (the network's WPA2 password protects it) |
| Home network, with **Access from home network** on (default) | `https://<ESP32 IP>` or `https://ps5-wake-xxxx.local` | HTTP Basic: the **panel user name** (default `admin`) and the **panel password** |

- With **Encrypted connection (HTTPS)** switched off, the home network uses `http://` instead.
- Plain `http://` requests are answered with `301` to the `https://` address (when HTTPS is on).
- The certificate is self-signed and created on the device: tell your client not to verify it (`curl -k`,
  `verify_ssl: false`) or trust the certificate downloaded from `/download/ps5-wake.crt`.
- `503` with `Retry-After` means HTTPS is starting or the certificate is being replaced – try again shortly.
- Wrong passwords get `401` after a 1-second delay.

## Endpoints

All responses are JSON unless noted. Request bodies are JSON (`Content-Type: application/json`).

### `POST /api/wake`

Starts a wake attempt (same as the BOOT button or a Wake-on-LAN packet).

```json
{"accepted": true}
```

`accepted: false` means an attempt is already running or the previous one was less than 30 seconds ago. The result
of the attempt appears a few seconds later in `last_wake` of `/api/status`.

### `GET /api/status`

Current state and settings (passwords are never returned).

| Field | Meaning |
|---|---|
| `version` | firmware version |
| `configured` | Wi-Fi and Bluetooth addresses are set |
| `ssid`, `wifi_connected`, `wifi_reason`, `ip`, `rssi` | home Wi-Fi state (`wifi_reason`: last disconnect reason code) |
| `esp_mac`, `wol_mac` | the ESP32's Wi-Fi MAC; MAC that Wake-on-LAN packets must target |
| `ps5_bt`, `controller_bt` | Bluetooth addresses of the console and the controller |
| `wol_ps5` | optional PS5 network MAC also accepted for Wake-on-LAN |
| `country`, `country_set`, `country_active` | Wi-Fi country setting (`""` = automatic, from the router), whether it was ever set, the code the Wi-Fi driver uses now |
| `lang` | panel language |
| `wol_count` | accepted Wake-on-LAN packets since start |
| `busy` | a wake attempt is running |
| `ps5_power`, `ps5_name`, `ps5_ip` | console state from the PlayStation discovery protocol: `on`, `standby` (rest mode), `off`, `unknown` (not found yet); the console's name and IP when found |
| `last_wake.status` | `-1` none yet, `0` finished, `-2` waiting, other values: radio error |
| `last_wake.lmp_rx` | link-manager packets received from the console (`> 0` with `status 0` = the console answered) |
| `last_wake.seconds_ago` | seconds since the last attempt |
| `panel_user` | user name for the home-network panel |
| `device_name`, `hostname` | optional device name; mDNS name without `.local` (the device name, or `ps5-wake-` + the 4 hex digits of the panel network name) |
| `consoles` | PS5 consoles answering discovery in the last 10 minutes: `[{"mac","name","ip","state"}]` |
| `led_dark` | LED completely off (only the BOOT hold feedback is shown) |
| `led_idle_off`, `portal_always`, `https` | panel options (LED off when idle, access from home network, HTTPS) |
| `ap_custom` | an own panel password is set |
| `ap_ssid`, `portal_seconds_left`, `via_ap`, `secure` | panel network name, time until it closes, request came through the panel network / over HTTPS |
| `cert_sha256`, `cert_expires`, `clock` | certificate fingerprint and expiry; the time is known from NTP |
| `mqtt_on`, `mqtt_url`, `mqtt_user`, `mqtt_pass_set`, `mqtt_connected`, `mqtt_id` | Home Assistant (MQTT) settings and state |

### `POST /api/save`

Saves all settings and restarts the ESP32. Body:

| Field | Required | Notes |
|---|---|---|
| `ssid` | yes | home Wi-Fi name |
| `password` | when the network changes | 8–64 characters; empty keeps the saved one for the same network |
| `open` | no | `true` for a network without a password |
| `ps5_bt`, `controller_bt` | yes | `AA:BB:CC:DD:EE:FF` |
| `wol_ps5` | no | empty removes it |
| `country` | no | ISO 3166 two-letter code, or `""` for automatic (from the router) |
| `ap_password` | until an own one is set | panel password, 8–63 characters, not the default |
| `device_name` | no | up to 24 characters; empty for none |
| `panel_user` | no | panel user name, 3–32 characters without `:`; default `admin` |
| `led_idle_off`, `led_dark`, `portal_always`, `https` | no | booleans |
| `mqtt_on`, `mqtt_url`, `mqtt_user`, `mqtt_pass` | no | `mqtt_url`: `mqtt://host:port` or `mqtts://host:port`; empty `mqtt_pass` keeps the saved one |
| `lang` | no | panel language |

Response: `{"ok": true}` or `{"ok": false, "error": "<code>"}` with codes such as `ssid`, `wifipass`, `needpass`,
`bt`, `btsame`, `wol`, `country`, `ap`, `appass`, `puser`, `devname`, `mqtt`, `nvs`.

### Other endpoints

| Method and path | Purpose |
|---|---|
| `GET /api/scan` | Wi-Fi networks in range: `{"networks":[{"ssid","rssi","channel","open"}]}` |
| `POST /api/lang` | `{"lang":"pl"}` – store the panel language |
| `POST /api/extend` | keep the panel network on for another 10 minutes |
| `POST /api/exit` | close the panel network now (restarts the ESP32; does nothing on the home network) |
| `POST /api/newcert` | create a new HTTPS certificate (HTTPS restarts within a few seconds) |
| `GET /download/ps5-wake.crt` | the device certificate (PEM) |
| `GET /download/ps5-wake-reader.cmd`, `GET /download/ps5_wake_reader.py` | controller address readers |
| `GET /` | the panel page |

## Examples

```sh
# wake the console from the home network (panel user name and password)
curl -k -u admin:PANEL_PASSWORD -X POST https://192.168.1.50/api/wake

# was it answered?
curl -k -u admin:PANEL_PASSWORD https://192.168.1.50/api/status | jq .last_wake
```

Home Assistant `configuration.yaml` (REST variant; the MQTT integration in
[home-assistant.md](../integrations/home-assistant.md) is easier):

```yaml
rest_command:
  ps5_wake:
    url: https://192.168.1.50/api/wake
    method: POST
    username: admin
    password: !secret ps5_wake_panel_password
    verify_ssl: false
```

## Serial console

Over USB at 115200 baud: `STATUS`, `WAKE`, `RESTART`, `SCAN`, `SCAN_ALL`, `COUNTRY <ISO2>`, `CONFIG <json>` (same
body as `/api/save`).

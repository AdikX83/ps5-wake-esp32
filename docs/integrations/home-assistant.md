# Home Assistant

There are three ways to use PS5 Wake in Home Assistant. **MQTT** is recommended: the device appears by itself, with
a wake button and the result of the last attempt.

## 1. MQTT (recommended)

What you get – a device **PS5 Wake** with:

| Entity | Type | Notes |
|---|---|---|
| Wake | button | starts a wake attempt |
| Last wake | sensor (enum) | `idle`, `waking`, `answered` (the console responded), `no_answer` |
| Console | sensor (enum) | `on`, `standby` (rest mode), `off`, `unknown` – see below |
| Wi-Fi signal | sensor (dBm) | diagnostic, every 60 s |

The device shows as unavailable when the ESP32 is offline (MQTT last will).

### Setup

1. In Home Assistant install the **Mosquitto broker** add-on (Settings → Add-ons) and add the **MQTT** integration
   (Settings → Devices & services). Without Home Assistant OS, any MQTT broker works.
2. Create a Home Assistant user for the ESP32 (Settings → People → Users), e.g. `ps5wake` – the Mosquitto add-on
   accepts Home Assistant users.
3. Open the PS5 Wake panel, section **Home Assistant (MQTT)**:
   - turn on **Connect to Home Assistant**,
   - **MQTT broker address**: `mqtt://192.168.1.10:1883` (the address of your Home Assistant),
   - user name and password from step 2,
   - **Save and restart**.
4. The panel status shows **Home Assistant: connected**, and the device appears under the MQTT integration.

`mqtts://host:8883` is supported for brokers with a certificate from a public authority.

### Console state

The **Console** sensor uses the PlayStation discovery protocol (the same one the PS App uses): the ESP32 asks on
UDP port 9302 every 10 seconds. `standby` is only reported when the console stays connected to the internet in rest
mode (Settings → System → Power saving → Features available in rest mode); otherwise rest mode shows as `off`. `off` is reported after 60 seconds without an answer, and a change between `on` and `standby` needs the same
answer twice in a row, so short gaps and background activity in rest mode do not show up as state changes. If
the PS5's network MAC is set in the panel, only that console counts.

### Automation example

```yaml
automation:
  - alias: Wake the PS5 when the TV turns on
    trigger:
      - platform: state
        entity_id: media_player.living_room_tv
        to: "on"
    action:
      - action: button.press
        target:
          entity_id: button.ps5_wake_wake
```

Entity IDs come from the device name and the entity name (here a device without a name, "PS5 Wake"); with the
device name "Living room PS5" the button is `button.living_room_ps5_wake`. Check them on the device page.

### Several devices

Every PS5 Wake is a separate device in the MQTT integration: unique IDs, topics and the MQTT client ID come from the
ESP32's MAC address, so any number of devices can share one broker and one Home Assistant user. Give each one a
**device name** in its panel (e.g. "Living room", "Kids room") – it becomes the device name in Home Assistant and the
base of the entity IDs. Devices without a name are all called "PS5 Wake"; Home Assistant then adds `_2`, `_3` to the
entity IDs. If several consoles are in the network, choose each device's console in its panel so the **Console**
sensor follows the right one.

### Topics

`<id>` is `ps5wake_` followed by the ESP32's Wi-Fi MAC in lower-case hex (returned by `/api/status` as `mqtt_id`).

| Topic | Direction | Payload |
|---|---|---|
| `ps5wake/<id>/wake` | to ESP32 | anything – starts a wake attempt |
| `ps5wake/<id>/result` | from ESP32 (retained) | `idle`, `waking`, `answered`, `no_answer` |
| `ps5wake/<id>/power` | from ESP32 (retained) | `on`, `standby`, `off`, `unknown` |
| `ps5wake/<id>/rssi` | from ESP32 | Wi-Fi signal in dBm |
| `ps5wake/<id>/availability` | from ESP32 (retained) | `online` / `offline` |
| `homeassistant/<component>/<id>/<object>/config` | from ESP32 (retained) | discovery |

Discovery is sent again when Home Assistant publishes `online` on `homeassistant/status`.

## 2. Wake on LAN integration (no setup on the ESP32)

Home Assistant's built-in [Wake on LAN](https://www.home-assistant.io/integrations/wake_on_lan/) integration can wake
the console through PS5 Wake: add the integration with the **ESP32's MAC address** shown in the panel (not the PS5's),
and use the created button in automations. There is no feedback; combine it with the
[Ping](https://www.home-assistant.io/integrations/ping/) integration on the console's IP address to see whether the
PS5 is on.

## 3. REST

With **Access from home network** on, Home Assistant can call the panel API – see [REST API](../technical/api.md#examples).
This needs the panel password in Home Assistant and `verify_ssl: false` because of the device's own certificate.

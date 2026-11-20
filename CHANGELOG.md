# Changelog

All notable changes are listed here. The project follows [semantic versioning](https://semver.org/).

## [1.0.3]

- Wake-up: Bluetooth now starts from scratch for every attempt and is switched off completely afterwards. After many
  hours of uptime the console stopped answering until the ESP32 was restarted; every attempt now starts from the same
  state as after a restart, and Wi-Fi no longer shares the radio with an idle Bluetooth controller.
- Wake-up: if the console does not answer, a second attempt follows at once.
- Wi-Fi power saving is switched off again after each Bluetooth use.
- `/api/status`: `wake_attempts`, `wake_answered`, `wake_retries`, `uptime_s`.
- Build: a change of `VERSION` now re-runs the configuration, so incremental builds never keep an old version string.

## [1.0.2]

- Panel: notice when a newer release is available on GitHub, with links to the release notes and the web installer.
  The check is made by the browser that shows the panel (not by the ESP32), at most every 6 hours, and can be turned
  off with "Check for updates" in the LED and panel section. "Hide" dismisses the notice for that version.
- Panel: on narrow screens (phones) the status labels are shown above the values and the panel address no longer
  runs past the edge of the card.
- README: screenshots of the setup panel and each of its sections.
- GitHub Actions: artifact and Pages actions updated to their Node.js 24 versions.

## [1.0.1]

Web installer fixes; the firmware is unchanged apart from the version number.

- The installer page no longer freezes when writing starts (a loop in the code that hides the "This will take
  2 minutes" estimate).
- The "Erase device" question is gone: a full chip erase from the browser hangs on the ESP32-C6's native USB port.
  The page has two buttons instead: **Install or update** keeps the settings, **Clean install** deletes them by
  writing an empty settings area.

## [1.0.0]

First public release.

- Wakes a PS5 from rest mode and from fully off over Bluetooth Classic (Soft-BR on the ESP32-C6), acting as the
  paired DualSense.
- Wake-on-LAN: UDP magic packets on ports 9 and 7, and raw Ethernet frames (EtherType 0x0842) as sent by routers
  such as Asuswrt-Merlin. Reacts to the ESP32's own MAC and, optionally, to the PS5's network MAC.
- BOOT button: short press wakes, 5–20 s opens the setup panel, 20 s erases all settings; safe mode when held at
  power-on.
- Setup panel in 30 languages: Wi-Fi scan, country, Bluetooth addresses via a setup code, WoL options, LED option,
  "Wake now" test, status and extend; HTTPS with a device-generated certificate; `ps5-wake-xxxx.local`.
- "Access from home network" (on by default): the panel is served on the home network behind the panel user name (default `admin`) and password, without the access point;
  HTTPS by default with a device-generated certificate (NTP-dated, 825 days, renewed automatically).
- A new or reset device starts the setup panel network by itself.
- Console state (on / rest mode / off) from the PlayStation discovery protocol: in the panel, the API and Home
  Assistant.
- Unique address per device (`<device-name>.local` or `ps5-wake-xxxx.local`), optional device name, choice of
  the console when several answer in the network.
- Home Assistant integration over MQTT discovery (wake button, last result, Wi-Fi signal); REST API documentation.
- Wi-Fi country: "Automatic (from the router)" by default, suggested from the browser, with a warning for networks on
  channels 12–13.
- An own panel password is required before saving.
- Address reader for Windows (no installation), macOS and Linux, downloadable from the panel.
- RGB LED status colours; the LED can be off when ready or off completely.
- Wi-Fi reconnect handling for WPA3 access points and an offline watchdog.
- Web installer, CI, user guides in 30 languages.

# Security

## Reporting a vulnerability

Please report security problems privately through
[GitHub security advisories](https://github.com/AdikX83/ps5-wake-esp32/security/advisories/new), not in public issues.
You should get an answer within two weeks.

## Security model

PS5 Wake is a hobby device for a home network. Keep in mind:

- **Setup panel network.** The panel's access point uses WPA2. The default password is public, so the firmware
  refuses to save settings until you set your own.
- **Home network.** By default the panel accepts connections only through its own access point and refuses them
  from the home network. With "Access from home network" the access point is not started; instead the panel is served on the
  home network and every request needs the panel user name and password (HTTP Basic authentication).
- **HTTPS.** The panel uses HTTPS with a self-signed certificate generated on the device – always in the panel
  network (on top of WPA2), and by default on the home network ("Access from home network", itself on by default – switch it off to keep the
  panel off your home network). The browser warns on the
  first visit – compare the SHA-256
  fingerprint shown in the panel (or on the serial console) before continuing, or install the downloaded certificate.
  If you switch HTTPS off for the home network, the password and the settings travel unencrypted inside your LAN. Use a panel password you
  do not use anywhere else.
- **Certificate key.** The TLS private key is stored unencrypted in NVS like the other settings; a factory reset
  erases it, and "New certificate" in the panel replaces it.
- **Wake-on-LAN** has no authentication by design: any device on your network can wake the console, just as with the
  console's own network wake features.
- **Stored data.** Wi-Fi credentials and Bluetooth addresses are stored unencrypted in the ESP32's flash (NVS).
  Hold BOOT for 20 seconds before giving the device away.
- **Bluetooth.** The device uses the controller's and console's addresses only to page the console; it does not
  store or use any pairing keys.

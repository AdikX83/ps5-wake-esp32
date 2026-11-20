# PS5 Wake – user guide

PS5 Wake is a small ESP32-C6 board that turns on your PlayStation 5 when it receives a Wake-on-LAN packet or when
you press its button. Over Bluetooth it acts as your paired DualSense controller, just like pressing the PS
button. It works from rest mode and from a fully switched-off console.

## What you need

- An ESP32-C6 board with PS5 Wake (tested: ESP32-C6-DevKitM-1), powered from any USB charger.
- A DualSense (or DualSense Edge) controller already paired with your PS5.
- A computer and a USB **data** cable – only once, to install the firmware and read the controller addresses.
- A 2.4 GHz Wi-Fi network.

## 1. Install the firmware

Open the web installer (link in the project README) in Chrome or Edge, connect the board with a USB cable and click
**Install or update**. A new device lights **purple**. It starts the setup panel network by itself.

## 2. Open the setup panel

1. Hold the **BOOT** button for 5–20 seconds and release it. The LED turns **red**. A new device has the panel on already (purple LED) – skip this step.
2. On the computer you will plug the controller into, connect to the Wi-Fi network **`PS5-WAKE-xxxx`** with the
   password **`ps5wol-setup`**.
3. In the browser open **`https://192.168.4.1`** (or `https://ps5-wake-xxxx.local`). The browser warns that the connection is not private, because the ESP32 uses its own certificate – choose **Advanced → Continue** (the wording depends on the browser).
4. Choose your language at the top of the page.

## 3. Read the controller addresses

1. Connect the DualSense to the computer with the USB cable.
2. In the **Bluetooth** section of the panel download the address reader:
   - **Windows:** `ps5-wake-reader.cmd` – double-click it. If Windows warns about an unknown publisher, choose
     **More info → Run anyway**.
   - **macOS:** run `pip3 install hidapi`, then `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (no extra packages needed).
3. The reader shows the Bluetooth addresses of the console and of the controller and copies a **setup code**
   (`PS5WAKE:…`) to the clipboard. Paste it into the **Setup code** field – both addresses are filled in.

The addresses change only when you pair the controller with the console again.

## 4. Finish the setup

1. Choose your Wi-Fi network with **Scan**, enter its password and select your country.
2. Enter a **new password for the panel network** (at least 8 characters). It is required, because the default
   password is publicly known.
3. Click **Save and restart**. The panel closes, the LED blinks **blue** while connecting and turns **green** when
   ready.

## 5. Set up Wake-on-LAN

- In your Wake-on-LAN app (or your router's WoL page) use the **ESP32 MAC address** shown in the panel, UDP port
  **9** and the **broadcast address** of your network (for example `192.168.1.255`).
- Reserve a fixed IP address for the ESP32 in your router.
- If your app or router already uses the **PS5's own network MAC**, you can enter it in the panel – the ESP32 then
  reacts to it too. In mesh networks (for example ASUS AiMesh) such packets may not reach the ESP32; in that case
  send WoL to the ESP32's MAC.

## Everyday use

- **Console state:** the dot on the power icon at the top of the panel is green when the PS5 is on, orange in rest mode and red when it is off or not reachable (gray: not found yet). In rest mode it is only detected when the console stays connected to the internet.
- **Wake the PS5:** send a Wake-on-LAN packet or briefly press **BOOT**. The LED flashes white, blinks cyan during
  the attempt, then flashes **green 3×** (the console answered) or **red 3×** (no answer).
- Attempts are at least 30 seconds apart; an ignored request flashes orange once.

## The BOOT button

| Hold time | LED while holding | Result |
|---|---|---|
| under 1 s | – | wake the PS5 |
| 1–5 s | white | nothing (cancel) |
| 5–20 s, then release | red (blinking from 15 s) | open the setup panel |
| 20 s | purple, fast blinking | erase all settings and restart |

Holding BOOT while plugging in the power starts a safe mode without Wi-Fi and Bluetooth (yellow LED).

## LED colours

| Colour | Behaviour | Meaning |
|---|---|---|
| purple | steady | not configured – the setup panel is on |
| blue | slow blink | connecting to Wi-Fi |
| orange | slow blink | Wi-Fi set up but not connected for over 30 s |
| green (dim) | steady | ready (can be switched off in the panel) |
| red | steady | setup panel is on |
| yellow | steady | safe mode |
| white | one flash | Wake-on-LAN received or BOOT pressed |
| cyan | fast blink | wake attempt in progress |
| green | three flashes | the console answered |
| red | three flashes | the console did not answer |
| orange | one flash | request ignored (attempt running or 30 s pause) |

## The setup panel

- The panel and its network turn off after saving, after **Close panel without changes**, or after 10 minutes
  without any action. **Extend (+10 min)** gives more time.
- **Turn the LED off completely**: No colours, blinks or flashes at all. Only while you hold BOOT does the LED still show the hold time.
- **Device name (optional)**: E.g. Living room. Used in the panel address (`living-room.local`), in Home Assistant and in the page title – helpful with several devices.
- **Access from home network**: On by default. The page stays available in your home network at `<ESP32 IP address>` and `ps5-wake-xxxx.local` – log in with the panel password (user name admin unless you changed it). The panel Wi-Fi network is not started; hold BOOT for 5 s to open it.
- **Encrypted connection (HTTPS)**: Recommended. The ESP32 creates its own certificate, so on the first visit the browser warns that the connection is not private – compare the fingerprint below and continue. Without HTTPS the password and settings travel unencrypted in your network. The certificate is renewed automatically before it expires (the ESP32 gets the time from the internet); **New certificate** creates a new one right away.
- **Wake PS5 now** tests the wake-up from the panel.

## Troubleshooting

- **Orange blinking LED:** wrong Wi-Fi password, a 5 GHz-only network or weak signal. Open the panel, check the
  Wi-Fi settings and move the ESP32 closer to the router. The ESP32 restarts by itself if it stays offline for
  2 minutes.
- **Nothing happens after sending Wake-on-LAN (no white flash):** the packet does not reach the ESP32. Use the
  ESP32's MAC, the broadcast address and port 9; check that the ESP32 is not on a guest network.
- **Red flashes after a wake attempt:** check the Bluetooth addresses (read them again if you re-paired the
  controller) and move the ESP32 closer to the console.
- **The reader finds no controller:** use a cable that carries data (not only power) and connect the controller
  directly to the computer.
- **The browser warns about the certificate:** this is expected – the panel is encrypted with the ESP32's own certificate. Continue to the page; the panel shows the certificate fingerprint.

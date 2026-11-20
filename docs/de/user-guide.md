# PS5 Wake – Benutzerhandbuch

PS5 Wake ist eine kleine ESP32-C6-Platine, die deine PlayStation 5 einschaltet, wenn sie ein Wake-on-LAN-Paket
empfängt oder wenn du ihre Taste drückst. Über Bluetooth gibt sie sich als dein gekoppelter DualSense-Controller aus –
wie ein Druck auf die PS-Taste. Das funktioniert aus dem Ruhemodus und bei vollständig ausgeschalteter Konsole.

## Was du brauchst

- Eine ESP32-C6-Platine mit PS5 Wake (getestet: ESP32-C6-DevKitM-1), versorgt über ein beliebiges USB-Netzteil.
- Einen DualSense- (oder DualSense-Edge-)Controller, der bereits mit deiner PS5 gekoppelt ist.
- Einen Computer und ein USB-**Datenkabel** – nur einmal, zum Installieren der Firmware und Auslesen der
  Controller-Adressen.
- Ein 2,4-GHz-WLAN.

## 1. Firmware installieren

Öffne den Web-Installer (Link im README des Projekts) in Chrome oder Edge, verbinde die Platine per USB-Kabel und
klicke auf **Install**. Ein neues Gerät leuchtet **lila**. Es startet das Netzwerk des Einrichtungspanels von selbst.

## 2. Einrichtungspanel öffnen

1. Halte die Taste **BOOT** 5–20 Sekunden gedrückt und lass sie los. Die LED leuchtet **rot**. Bei einem neuen Gerät ist das Panel bereits aktiv (lila LED) – überspringe diesen Schritt.
2. Verbinde dich auf dem Computer, an den du den Controller anschließen wirst, mit dem WLAN **`PS5-WAKE-xxxx`**,
   Passwort **`ps5wol-setup`**.
3. Öffne im Browser **`https://192.168.4.1`** (oder `https://ps5-wake-xxxx.local`). Der Browser warnt, dass die Verbindung nicht privat ist, weil der ESP32 ein eigenes Zertifikat verwendet – wähle **Erweitert → Weiter** (die Bezeichnung hängt vom Browser ab).
4. Wähle oben auf der Seite deine Sprache.

## 3. Controller-Adressen auslesen

1. Schließe den DualSense per USB-Kabel an den Computer an.
2. Lade im Abschnitt **Bluetooth** des Panels das Adress-Leseprogramm herunter:
   - **Windows:** `ps5-wake-reader.cmd` – per Doppelklick starten. Warnt Windows vor einem unbekannten Herausgeber,
     wähle **Weitere Informationen → Trotzdem ausführen**.
   - **macOS:** `pip3 install hidapi` ausführen, dann `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (keine zusätzlichen Pakete nötig).
3. Das Programm zeigt die Bluetooth-Adressen von Konsole und Controller und kopiert einen **Einrichtungscode**
   (`PS5WAKE:…`) in die Zwischenablage. Füge ihn in das Feld **Einrichtungscode aus dem Leseprogramm** ein – beide
   Adressen werden ausgefüllt.

Die Adressen ändern sich nur, wenn du den Controller erneut mit der Konsole koppelst.

## 4. Einrichtung abschließen

1. Wähle dein WLAN mit **Suchen**, gib das Passwort ein und wähle dein Land.
2. Gib ein **neues Passwort für das Panel-Netzwerk** ein (mindestens 8 Zeichen). Es ist erforderlich, weil das
   Standardpasswort öffentlich bekannt ist.
3. Klicke auf **Speichern und neu starten**. Das Panel schließt sich, die LED blinkt beim Verbinden **blau** und
   leuchtet **grün**, wenn alles bereit ist.

## 5. Wake-on-LAN einrichten

- Verwende in deiner Wake-on-LAN-App (oder auf der WoL-Seite deines Routers) die im Panel angezeigte
  **MAC-Adresse des ESP32**, UDP-Port **9** und die **Broadcast-Adresse** deines Netzwerks (z. B. `192.168.1.255`).
- Reserviere im Router eine feste IP-Adresse für den ESP32.
- Verwendet deine App oder dein Router bereits die **Netzwerk-MAC der PS5 selbst**, kannst du sie im Panel
  eintragen – der ESP32 reagiert dann auch darauf. In Mesh-Netzwerken (z. B. ASUS AiMesh) erreichen solche Pakete den
  ESP32 eventuell nicht; sende WoL dann an die MAC des ESP32.

## Im Alltag

- **Konsolenstatus:** Der Punkt am Ein-/Aus-Symbol oben im Panel ist grün, wenn die PS5 eingeschaltet ist, orange im Ruhemodus und rot, wenn sie aus oder nicht erreichbar ist (grau: noch nicht gefunden). Der Ruhemodus wird nur erkannt, wenn die Konsole darin mit dem Internet verbunden bleibt.
- **PS5 aufwecken:** Sende ein Wake-on-LAN-Paket oder drücke kurz **BOOT**. Die LED blitzt weiß, blinkt während des
  Versuchs türkis und blitzt dann **3× grün** (die Konsole hat geantwortet) oder **3× rot** (keine Antwort).
- Zwischen Versuchen liegen mindestens 30 Sekunden; eine ignorierte Anfrage blitzt einmal orange.

## Die Taste BOOT

| Haltedauer | LED beim Halten | Ergebnis |
|---|---|---|
| unter 1 s | – | PS5 aufwecken |
| 1–5 s | weiß | nichts (Abbruch) |
| 5–20 s, dann loslassen | rot (ab 15 s blinkend) | Einrichtungspanel öffnen |
| 20 s | lila, schnell blinkend | alle Einstellungen löschen und neu starten |

Wird BOOT beim Anschließen der Stromversorgung gehalten, startet ein abgesicherter Modus ohne WLAN und Bluetooth
(gelbe LED).

## LED-Farben

| Farbe | Verhalten | Bedeutung |
|---|---|---|
| lila | leuchtet | nicht eingerichtet – Einrichtungspanel ist aktiv |
| blau | blinkt langsam | verbindet mit dem WLAN |
| orange | blinkt langsam | WLAN eingerichtet, aber seit über 30 s keine Verbindung |
| grün (gedimmt) | leuchtet | bereit (im Panel abschaltbar) |
| rot | leuchtet | Einrichtungspanel ist aktiv |
| gelb | leuchtet | abgesicherter Modus |
| weiß | einmal | Wake-on-LAN empfangen oder BOOT gedrückt |
| türkis | blinkt schnell | Aufweckversuch läuft |
| grün | dreimal | die Konsole hat geantwortet |
| rot | dreimal | die Konsole hat nicht geantwortet |
| orange | einmal | Anfrage ignoriert (Versuch läuft oder 30 s Pause) |

## Das Einrichtungspanel

- Panel und Panel-Netzwerk schalten sich nach dem Speichern, nach **Panel ohne Änderungen schließen** oder nach
  10 Minuten ohne Aktion ab. **Verlängern (+10 Min.)** gibt mehr Zeit.
- **LED vollständig ausschalten**: Keine Farben, kein Blinken und keine Blitze. Nur solange du BOOT hältst, zeigt die LED weiterhin die Haltedauer.
- **Gerätename (optional)**: Z. B. Wohnzimmer. Wird in der Panel-Adresse (`wohnzimmer.local`), in Home Assistant und im Seitentitel verwendet – hilfreich bei mehreren Geräten.
- **Zugriff aus dem Heimnetz**: Standardmäßig aktiviert. Die Seite bleibt im Heimnetz unter `<IP des ESP32>` und `ps5-wake-xxxx.local` erreichbar – Anmeldung mit dem Panel-Passwort (Benutzername admin, sofern nicht geändert). Das Panel-WLAN wird nicht gestartet; halte BOOT 5 s, um es zu öffnen.
- **Verschlüsselte Verbindung (HTTPS)**: Empfohlen. Der ESP32 erstellt ein eigenes Zertifikat, daher warnt der Browser beim ersten Besuch, dass die Verbindung nicht privat ist – vergleiche den Fingerabdruck unten und fahre fort. Ohne HTTPS werden Passwort und Einstellungen unverschlüsselt im Netzwerk übertragen. Das Zertifikat wird vor Ablauf automatisch erneuert (der ESP32 holt die Uhrzeit aus dem Internet); **Neues Zertifikat** erstellt sofort ein neues.
- **PS5 jetzt aufwecken** testet das Aufwecken direkt im Panel.

## Fehlerbehebung

- **LED blinkt orange:** falsches WLAN-Passwort, reines 5-GHz-Netz oder schwaches Signal. Öffne das Panel, prüfe die
  WLAN-Einstellungen und stelle den ESP32 näher an den Router. Der ESP32 startet von selbst neu, wenn er 2 Minuten
  offline ist.
- **Nach dem Senden von Wake-on-LAN passiert nichts (kein weißer Blitz):** Das Paket erreicht den ESP32 nicht.
  Verwende die MAC des ESP32, die Broadcast-Adresse und Port 9; prüfe, ob der ESP32 nicht im Gastnetz ist.
- **Rote Blitze nach einem Aufweckversuch:** Prüfe die Bluetooth-Adressen (lies sie nach erneutem Koppeln des
  Controllers neu aus) und stelle den ESP32 näher an die Konsole.
- **Das Leseprogramm findet keinen Controller:** Verwende ein Datenkabel (nicht nur ein Ladekabel) und schließe den
  Controller direkt an den Computer an.
- **Der Browser warnt vor dem Zertifikat:** Das ist normal – das Panel ist mit dem eigenen Zertifikat des ESP32 verschlüsselt. Fahre zur Seite fort; das Panel zeigt den Fingerabdruck des Zertifikats.

# PS5 Wake – gebruikershandleiding

PS5 Wake is een klein ESP32-C6-bordje dat je PlayStation 5 aanzet zodra het een Wake-on-LAN-pakket ontvangt of
wanneer je op de knop drukt. Via Bluetooth doet het zich voor als je gekoppelde DualSense-controller – net als een druk
op de PS-knop. Het werkt vanuit de rustmodus en bij een volledig uitgeschakelde console.

## Wat je nodig hebt

- Een ESP32-C6-bordje met PS5 Wake (getest: ESP32-C6-DevKitM-1), gevoed door een willekeurige USB-lader.
- Een DualSense- (of DualSense Edge-)controller die al met je PS5 is gekoppeld.
- Een computer en een USB-**datakabel** – één keer, om de firmware te installeren en de controlleradressen uit te
  lezen.
- Een 2,4GHz-wifinetwerk.

## 1. Firmware installeren

Open de webinstaller (link in de README van het project) in Chrome of Edge, sluit het bordje met een USB-kabel aan en
klik op **Install**. Een nieuw apparaat brandt **paars**. Het start vanzelf het netwerk van het instelpaneel.

## 2. Het instelpaneel openen

1. Houd de knop **BOOT** 5–20 seconden ingedrukt en laat los. De led wordt **rood**. Bij een nieuw apparaat staat het paneel al aan (paarse led) – sla deze stap over.
2. Maak op de computer waarop je de controller aansluit verbinding met het wifinetwerk **`PS5-WAKE-xxxx`**,
   wachtwoord **`ps5wol-setup`**.
3. Open in de browser **`https://192.168.4.1`** (of `https://ps5-wake-xxxx.local`). De browser waarschuwt dat de verbinding niet privé is, omdat de ESP32 een eigen certificaat gebruikt – kies **Geavanceerd → Doorgaan** (de benaming verschilt per browser).
4. Kies bovenaan de pagina je taal.

## 3. De controlleradressen uitlezen

1. Sluit de DualSense met de USB-kabel aan op de computer.
2. Download in het onderdeel **Bluetooth** van het paneel de adreslezer:
   - **Windows:** `ps5-wake-reader.cmd` – dubbelklik erop. Waarschuwt Windows voor een onbekende uitgever, kies dan
     **Meer informatie → Toch uitvoeren**.
   - **macOS:** voer `pip3 install hidapi` uit en daarna `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (geen extra pakketten nodig).
3. De lezer toont de Bluetooth-adressen van de console en de controller en kopieert een **instelcode** (`PS5WAKE:…`)
   naar het klembord. Plak die in het veld **Instelcode uit de adreslezer** – beide adressen worden ingevuld.

De adressen veranderen alleen als je de controller opnieuw met de console koppelt.

## 4. Instellen afronden

1. Kies je wifinetwerk met **Zoeken**, voer het wachtwoord in en kies je land.
2. Voer een **nieuw wachtwoord voor het paneelnetwerk** in (minstens 8 tekens). Dat is verplicht, omdat het
   standaardwachtwoord openbaar is.
3. Klik op **Opslaan en herstarten**. Het paneel sluit, de led knippert **blauw** tijdens het verbinden en wordt
   **groen** als alles klaar is.

## 5. Wake-on-LAN instellen

- Gebruik in je Wake-on-LAN-app (of op de WoL-pagina van je router) het **MAC-adres van de ESP32** uit het paneel,
  UDP-poort **9** en het **broadcastadres** van je netwerk (bijv. `192.168.1.255`).
- Reserveer in je router een vast IP-adres voor de ESP32.
- Gebruikt je app of router al het **netwerk-MAC-adres van de PS5 zelf**, dan kun je dat in het paneel invoeren – de
  ESP32 reageert er dan ook op. In meshnetwerken (bijv. ASUS AiMesh) bereiken zulke pakketten de ESP32 mogelijk niet;
  stuur WoL dan naar het MAC-adres van de ESP32.

## Dagelijks gebruik

- **Consolestatus:** de stip op het aan/uit-pictogram bovenaan het paneel is groen als de PS5 aan staat, oranje in rustmodus en rood als hij uit of onbereikbaar is (grijs: nog niet gevonden). De rustmodus wordt alleen herkend als de console daarin met internet verbonden blijft.
- **PS5 wekken:** stuur een Wake-on-LAN-pakket of druk kort op **BOOT**. De led flitst wit, knippert turquoise tijdens
  de poging en flitst daarna **3× groen** (de console heeft geantwoord) of **3× rood** (geen antwoord).
- Tussen pogingen zit minstens 30 seconden; een genegeerd verzoek geeft één oranje flits.

## De knop BOOT

| Indruktijd | Led tijdens indrukken | Resultaat |
|---|---|---|
| korter dan 1 s | – | PS5 wekken |
| 1–5 s | wit | niets (annuleren) |
| 5–20 s, dan loslaten | rood (knippert vanaf 15 s) | instelpaneel openen |
| 20 s | paars, snel knipperend | alle instellingen wissen en herstarten |

Houd je BOOT ingedrukt tijdens het aansluiten van de voeding, dan start een veilige modus zonder wifi en Bluetooth
(gele led).

## Ledkleuren

| Kleur | Gedrag | Betekenis |
|---|---|---|
| paars | brandt | niet ingesteld – instelpaneel staat aan |
| blauw | knippert langzaam | verbinden met wifi |
| oranje | knippert langzaam | wifi ingesteld maar al meer dan 30 s geen verbinding |
| groen (gedimd) | brandt | gereed (uit te zetten in het paneel) |
| rood | brandt | instelpaneel staat aan |
| geel | brandt | veilige modus |
| wit | één flits | Wake-on-LAN ontvangen of BOOT ingedrukt |
| turquoise | knippert snel | wekpoging bezig |
| groen | drie flitsen | de console heeft geantwoord |
| rood | drie flitsen | de console heeft niet geantwoord |
| oranje | één flits | verzoek genegeerd (poging bezig of 30 s pauze) |

## Het instelpaneel

- Het paneel en het paneelnetwerk gaan uit na het opslaan, na **Sluiten zonder wijzigingen** of na 10 minuten zonder
  actie. **Verlengen (+10 min)** geeft meer tijd.
- **Led volledig uitschakelen**: Geen kleuren, knipperen of flitsen. Alleen terwijl je BOOT ingedrukt houdt, toont de led nog de indruktijd.
- **Apparaatnaam (optioneel)**: Bijv. Woonkamer. Gebruikt in het paneeladres (`woonkamer.local`), in Home Assistant en in de paginatitel – handig bij meerdere apparaten.
- **Toegang vanuit thuisnetwerk**: Standaard ingeschakeld. De pagina blijft in je thuisnetwerk bereikbaar op `<IP van de ESP32>` en `ps5-wake-xxxx.local` – aanmelden met het paneelwachtwoord (gebruikersnaam admin, tenzij je die hebt gewijzigd). Het wifinetwerk van het paneel wordt niet gestart; houd BOOT 5 s ingedrukt om het te openen.
- **Versleutelde verbinding (HTTPS)**: Aanbevolen. De ESP32 maakt een eigen certificaat, dus bij het eerste bezoek waarschuwt de browser dat de verbinding niet privé is – vergelijk de vingerafdruk hieronder en ga verder. Zonder HTTPS gaan wachtwoord en instellingen onversleuteld over je netwerk. Het certificaat wordt vóór het verlopen automatisch vernieuwd (de ESP32 haalt de tijd van internet); **Nieuw certificaat** maakt direct een nieuw.
- **PS5 nu wekken** test het wekken vanuit het paneel.

## Problemen oplossen

- **Led knippert oranje:** verkeerd wifiwachtwoord, alleen een 5GHz-netwerk of zwak signaal. Open het paneel, controleer
  de wifi-instellingen en zet de ESP32 dichter bij de router. De ESP32 herstart vanzelf als hij 2 minuten offline is.
- **Er gebeurt niets na het versturen van Wake-on-LAN (geen witte flits):** het pakket bereikt de ESP32 niet. Gebruik
  het MAC-adres van de ESP32, het broadcastadres en poort 9; controleer dat de ESP32 niet op een gastnetwerk zit.
- **Rode flitsen na een poging:** controleer de Bluetooth-adressen (lees ze opnieuw uit na opnieuw koppelen van de
  controller) en zet de ESP32 dichter bij de console.
- **De lezer vindt geen controller:** gebruik een datakabel (niet alleen een laadkabel) en sluit de controller
  rechtstreeks op de computer aan.
- **De browser waarschuwt voor het certificaat:** dat is normaal – het paneel is versleuteld met het eigen certificaat van de ESP32. Ga door naar de pagina; het paneel toont de vingerafdruk van het certificaat.

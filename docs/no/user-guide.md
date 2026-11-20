# PS5 Wake – brukerveiledning

PS5 Wake er et lite ESP32-C6-kort som slår på PlayStation 5 når det mottar en Wake-on-LAN-pakke, eller når du trykker på knappen. Via Bluetooth utgir det seg for å være den sammenkoblede DualSense-kontrolleren din – akkurat som et trykk på PS-knappen. Det fungerer fra hvilemodus og fra en helt avslått konsoll.

## Dette trenger du

- Et ESP32-C6-kort med PS5 Wake (testet: ESP32-C6-DevKitM-1), drevet av en hvilken som helst USB-lader.
- En DualSense- (eller DualSense Edge-)kontroller som allerede er sammenkoblet med PS5.
- En datamaskin og en USB-**datakabel** – bare én gang, for å installere fastvaren og lese av kontrollerens adresser.
- Et Wi-Fi-nettverk på 2,4 GHz.

## 1. Installer fastvaren

Åpne nettinstallasjonsprogrammet (lenke i prosjektets README) i Chrome eller Edge, koble til kortet med en USB-kabel og klikk **Install or update**. En ny enhet lyser **lilla**. Det starter oppsettpanelets nettverk av seg selv.

## 2. Åpne oppsettpanelet

1. Hold **BOOT**-knappen inne i 5–20 sekunder og slipp. LED-en lyser **rødt**. På en ny enhet er panelet allerede på (lilla LED) – hopp over dette trinnet.
2. På datamaskinen du skal koble kontrolleren til, kobler du til Wi-Fi-nettverket `PS5-WAKE-xxxx`, passord `ps5wol-setup`.
3. Åpne **`https://192.168.4.1`** (eller `https://ps5-wake-xxxx.local`) i nettleseren. Nettleseren advarer om at tilkoblingen ikke er privat, fordi ESP32 bruker sitt eget sertifikat – velg **Avansert → Fortsett** (navnene avhenger av nettleseren).
4. Velg språk øverst på siden.

## 3. Les av kontrollerens adresser

1. Koble DualSense til datamaskinen med USB-kabelen.
2. Last ned adresseleseren i delen **Bluetooth** i panelet:
   - **Windows:** `ps5-wake-reader.cmd` – dobbeltklikk på den. Hvis Windows advarer om en ukjent utgiver, velger du **Mer informasjon → Kjør likevel**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Leseren viser Bluetooth-adressene til konsollen og kontrolleren og kopierer en **oppsettkode** (`PS5WAKE:…`) til utklippstavlen. Lim den inn i feltet **Oppsettskode fra adresseleseren** – begge adressene fylles ut.

Adressene endres bare hvis du kobler kontrolleren sammen med konsollen på nytt.

## 4. Fullfør oppsettet

1. Velg Wi-Fi-nettverket med **Søk**, skriv inn passordet og velg land.
2. Skriv inn **Nytt passord for panelets nettverk** (minst 8 tegn). Det er påkrevd fordi standardpassordet er offentlig kjent.
3. Klikk **Lagre og start på nytt**. Panelet lukkes, LED-en blinker **blått** under tilkoblingen og lyser **grønt** når alt er klart.

## 5. Sett opp Wake-on-LAN

- Bruk **MAC-adressen til denne ESP32** fra panelet, UDP-port **9** og nettverkets **kringkastingsadresse** (f.eks. `192.168.1.255`) i Wake-on-LAN-appen (eller på ruterens WoL-side).
- Reserver en fast IP-adresse for ESP32 i ruteren.
- Hvis appen eller ruteren allerede bruker **PS5-ens egen nettverks-MAC**, kan du skrive den inn i feltet **Reager også på PS5-ens egen MAC-adresse** – ESP32 reagerer da også på den. I mesh-nettverk (f.eks. ASUS AiMesh) når slike pakker kanskje ikke fram til ESP32; send i så fall WoL til MAC-adressen til ESP32.

## Daglig bruk

- **Konsollstatus:** prikken på strømikonet øverst i panelet er grønn når PS5 er på, oransje i hvilemodus og rød når den er av eller ikke kan nås (grå: ikke funnet ennå). Hvilemodus oppdages bare hvis konsollen forblir koblet til internett.
- **Vekk PS5:** send en Wake-on-LAN-pakke eller trykk kort på **BOOT**. LED-en blinker hvitt én gang, blinker turkis under forsøket og deretter **3× grønt** (konsollen svarte) eller **3× rødt** (ikke svar).
- Det går minst 30 sekunder mellom forsøkene; en ignorert forespørsel gir ett oransje blink.

## BOOT-knappen

| Holdetid | LED mens du holder | Resultat |
|---|---|---|
| under 1 s | – | vekk PS5 |
| 1–5 s | hvit | ingenting (avbryt) |
| 5–20 s, slipp deretter | rød (blinker fra 15 s) | åpne oppsettpanelet |
| 20 s | lilla, blinker raskt | slett alle innstillinger og start på nytt |

Holder du BOOT inne mens strømmen kobles til, starter en sikker modus uten Wi-Fi og Bluetooth (gul LED).

## LED-farger

| Farge | Oppførsel | Betydning |
|---|---|---|
| lilla | lyser | ikke satt opp – oppsettpanelet er på |
| blå | blinker sakte | kobler til Wi-Fi |
| oransje | blinker sakte | Wi-Fi satt opp, men ingen tilkobling på over 30 s |
| grønn (dempet) | lyser | klar (kan slås av i panelet) |
| rød | lyser | oppsettpanelet er på |
| gul | lyser | sikker modus |
| hvit | ett blink | Wake-on-LAN mottatt eller BOOT trykket |
| turkis | blinker raskt | vekkeforsøk pågår |
| grønn | tre blink | konsollen svarte |
| rød | tre blink | konsollen svarte ikke |
| oransje | ett blink | forespørsel ignorert (forsøk pågår eller 30 s pause) |

## Oppsettpanelet

- Panelet og nettverket slås av etter lagring, etter **Lukk uten endringer** eller etter 10 minutter uten aktivitet. **Forleng** gir mer tid.
- **Slå av LED-en helt**: Ingen farger, blinking eller blink. Bare mens du holder BOOT inne, viser LED-en fortsatt holdetiden.
- **Enhetsnavn (valgfritt)**: F.eks. Stue. Brukes i paneladressen (`stue.local`), i Home Assistant og i sidetittelen – nyttig med flere enheter.
- **Tilgang fra hjemmenettverket**: På som standard. Siden er alltid tilgjengelig i hjemmenettverket på `<ESP32s IP>` og `ps5-wake-xxxx.local` – logg inn med panelpassordet (brukernavn admin hvis du ikke har endret det). Panelets Wi-Fi-nettverk startes ikke; hold BOOT i 5 s for å åpne det.
- **Kryptert tilkobling (HTTPS)**: Anbefales. ESP32 lager sitt eget sertifikat, så ved første besøk advarer nettleseren om at tilkoblingen ikke er privat – sammenlign fingeravtrykket nedenfor og fortsett. Uten HTTPS sendes passord og innstillinger ukryptert i nettverket. Sertifikatet fornyes automatisk før det utløper (ESP32 henter tiden fra internett); **Nytt sertifikat** lager et nytt med en gang.
- **Vekk PS5 nå** tester vekkingen direkte fra panelet.

## Feilsøking

- **LED-en blinker oransje:** feil Wi-Fi-passord, bare 5 GHz-nettverk eller svakt signal. Åpne panelet, sjekk Wi-Fi og flytt ESP32 nærmere ruteren. ESP32 starter på nytt av seg selv hvis den er frakoblet i 2 minutter.
- **Ingenting skjer etter at Wake-on-LAN er sendt (ikke noe hvitt blink):** pakken når ikke fram til ESP32. Bruk MAC-adressen til ESP32, kringkastingsadressen og port 9; sjekk at ESP32 ikke er på et gjestenettverk.
- **Røde blink etter et forsøk:** sjekk Bluetooth-adressene (les dem av på nytt etter ny sammenkobling av kontrolleren) og flytt ESP32 nærmere konsollen.
- **Leseren finner ingen kontroller:** bruk en datakabel (ikke bare en ladekabel) og koble kontrolleren direkte til datamaskinen.
- **Nettleseren advarer om sertifikatet:** det er forventet – panelet krypteres med ESP32s eget sertifikat. Fortsett til siden; panelet viser sertifikatets fingeravtrykk.

# PS5 Wake – brugervejledning

PS5 Wake er et lille ESP32-C6-board, der tænder din PlayStation 5, når det modtager en Wake-on-LAN-pakke, eller når du trykker på dets knap. Via Bluetooth udgiver det sig for at være din parrede DualSense-controller – præcis som et tryk på PS-knappen. Det virker fra hvilemodus og fra en helt slukket konsol.

## Det skal du bruge

- Et ESP32-C6-board med PS5 Wake (testet: ESP32-C6-DevKitM-1), forsynet fra en hvilken som helst USB-oplader.
- En DualSense- (eller DualSense Edge-)controller, der allerede er parret med din PS5.
- En computer og et USB-**datakabel** – kun én gang, til at installere firmwaren og aflæse controllerens adresser.
- Et 2,4 GHz Wi-Fi-netværk.

## 1. Installer firmwaren

Åbn webinstallationsprogrammet (link i projektets README) i Chrome eller Edge, tilslut boardet med et USB-kabel, og klik på **Install**. En ny enhed lyser **lilla**. Det starter selv opsætningspanelets netværk.

## 2. Åbn opsætningspanelet

1. Hold knappen **BOOT** inde i 5–20 sekunder, og slip den. LED'en lyser **rødt**. På en ny enhed er panelet allerede tændt (lilla LED) – spring dette trin over.
2. På den computer, du vil slutte controlleren til, skal du oprette forbindelse til Wi-Fi-netværket `PS5-WAKE-xxxx`, adgangskode `ps5wol-setup`.
3. Åbn **`https://192.168.4.1`** (eller `https://ps5-wake-xxxx.local`) i browseren. Browseren advarer om, at forbindelsen ikke er privat, fordi ESP32 bruger sit eget certifikat – vælg **Avanceret → Fortsæt** (navnene afhænger af browseren).
4. Vælg sprog øverst på siden.

## 3. Aflæs controllerens adresser

1. Slut DualSense til computeren med USB-kablet.
2. Hent adresselæseren i afsnittet **Bluetooth** i panelet:
   - **Windows:** `ps5-wake-reader.cmd` – dobbeltklik på den. Hvis Windows advarer om en ukendt udgiver, vælg **Flere oplysninger → Kør alligevel**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Læseren viser konsollens og controllerens Bluetooth-adresser og kopierer en **opsætningskode** (`PS5WAKE:…`) til udklipsholderen. Indsæt den i feltet **Opsætningskode fra adresselæseren** – begge adresser udfyldes.

Adresserne ændres kun, hvis du parrer controlleren med konsollen igen.

## 4. Afslut opsætningen

1. Vælg dit Wi-Fi-netværk med **Søg**, indtast adgangskoden, og vælg dit land.
2. Indtast **Ny adgangskode til panelnetværket** (mindst 8 tegn). Den er påkrævet, fordi standardadgangskoden er offentligt kendt.
3. Klik på **Gem og genstart**. Panelet lukker, LED'en blinker **blåt** under forbindelsen og lyser **grønt**, når alt er klar.

## 5. Opsæt Wake-on-LAN

- Brug **MAC for denne ESP32** fra panelet, UDP-port **9** og netværkets **broadcastadresse** (f.eks. `192.168.1.255`) i din Wake-on-LAN-app (eller på routerens WoL-side).
- Reservér en fast IP-adresse til ESP32 i routeren.
- Hvis din app eller router allerede bruger **PS5'ens egen netværks-MAC**, kan du indtaste den i feltet **Reagér også på PS5’ens egen MAC** – ESP32 reagerer så også på den. I mesh-netværk (f.eks. ASUS AiMesh) når sådanne pakker måske ikke frem til ESP32; send i så fald WoL til ESP32's MAC.

## I hverdagen

- **Konsolstatus:** prikken på tænd/sluk-ikonet øverst i panelet er grøn, når PS5 er tændt, orange i hviletilstand og rød, når den er slukket eller ikke kan nås (grå: ikke fundet endnu). Hviletilstand registreres kun, hvis konsollen forbliver forbundet til internettet.
- **Væk PS5:** send en Wake-on-LAN-pakke, eller tryk kort på **BOOT**. LED'en blinker hvidt én gang, blinker turkis under forsøget og derefter **3× grønt** (konsollen svarede) eller **3× rødt** (intet svar).
- Der går mindst 30 sekunder mellem forsøg; en ignoreret anmodning giver ét orange blink.

## Knappen BOOT

| Holdetid | LED mens den holdes | Resultat |
|---|---|---|
| under 1 s | – | væk PS5 |
| 1–5 s | hvid | intet (annuller) |
| 5–20 s, derefter slip | rød (blinker fra 15 s) | åbn opsætningspanelet |
| 20 s | lilla, blinker hurtigt | slet alle indstillinger og genstart |

Holder du BOOT inde, mens strømmen tilsluttes, starter en sikker tilstand uden Wi-Fi og Bluetooth (gul LED).

## LED-farver

| Farve | Adfærd | Betydning |
|---|---|---|
| lilla | lyser | ikke opsat – opsætningspanelet er tændt |
| blå | blinker langsomt | forbinder til Wi-Fi |
| orange | blinker langsomt | Wi-Fi opsat, men ingen forbindelse i over 30 s |
| grøn (dæmpet) | lyser | klar (kan slås fra i panelet) |
| rød | lyser | opsætningspanelet er tændt |
| gul | lyser | sikker tilstand |
| hvid | ét blink | Wake-on-LAN modtaget eller BOOT trykket |
| turkis | blinker hurtigt | vækningsforsøg i gang |
| grøn | tre blink | konsollen svarede |
| rød | tre blink | konsollen svarede ikke |
| orange | ét blink | anmodning ignoreret (forsøg i gang eller 30 s pause) |

## Opsætningspanelet

- Panelet og dets netværk slukker efter gem, efter **Luk uden ændringer** eller efter 10 minutter uden aktivitet. **Forlæng** giver mere tid.
- **Sluk LED'en helt**: Ingen farver, blink eller glimt. Kun mens du holder BOOT inde, viser LED'en stadig holdetiden.
- **Enhedsnavn (valgfrit)**: F.eks. Stue. Bruges i panelets adresse (`stue.local`), i Home Assistant og i sidetitlen – nyttigt med flere enheder.
- **Adgang fra hjemmenetværket**: Slået til som standard. Siden er altid tilgængelig på dit hjemmenetværk på `<ESP32's IP>` og `ps5-wake-xxxx.local` – log ind med panelets adgangskode (brugernavn admin, medmindre du har ændret det). Panelets Wi-Fi-netværk startes ikke; hold BOOT i 5 s for at åbne det.
- **Krypteret forbindelse (HTTPS)**: Anbefales. ESP32 laver sit eget certifikat, så ved første besøg advarer browseren om, at forbindelsen ikke er privat – sammenlign fingeraftrykket nedenfor, og fortsæt. Uden HTTPS sendes adgangskode og indstillinger ukrypteret på netværket. Certifikatet fornys automatisk, før det udløber (ESP32 henter tiden fra internettet); **Nyt certifikat** laver straks et nyt.
- **Væk PS5 nu** tester vækningen direkte fra panelet.

## Fejlfinding

- **LED'en blinker orange:** forkert Wi-Fi-adgangskode, kun et 5 GHz-netværk eller svagt signal. Åbn panelet, tjek Wi-Fi, og flyt ESP32 tættere på routeren. ESP32 genstarter af sig selv, hvis den er offline i 2 minutter.
- **Intet sker efter afsendelse af Wake-on-LAN (intet hvidt blink):** pakken når ikke frem til ESP32. Brug ESP32's MAC, broadcastadressen og port 9; tjek, at ESP32 ikke er på et gæstenetværk.
- **Røde blink efter et forsøg:** tjek Bluetooth-adresserne (aflæs dem igen efter ny parring af controlleren), og flyt ESP32 tættere på konsollen.
- **Læseren finder ingen controller:** brug et datakabel (ikke kun et opladerkabel), og slut controlleren direkte til computeren.
- **Browseren advarer om certifikatet:** det er forventet – panelet er krypteret med ESP32's eget certifikat. Fortsæt til siden; panelet viser certifikatets fingeraftryk.

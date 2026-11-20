# PS5 Wake – kasutusjuhend

PS5 Wake on väike ESP32-C6 plaat, mis lülitab PlayStation 5 sisse, kui see saab Wake-on-LAN paketi või kui vajutate selle nuppu. Bluetoothi kaudu esitleb see end teie seotud DualSense'i puldina – täpselt nagu PS-nupu vajutus. Töötab puhkerežiimist ja täielikult välja lülitatud konsoolist.

## Mida on vaja

- ESP32-C6 plaat PS5 Wake'iga (testitud: ESP32-C6-DevKitM-1), toide suvalisest USB-laadijast.
- DualSense'i (või DualSense Edge'i) pult, mis on juba PS5-ga seotud.
- Arvuti ja **andmesidet toetav** USB-kaabel – ainult üks kord, püsivara paigaldamiseks ja puldi aadresside lugemiseks.
- 2,4 GHz Wi-Fi võrk.

## 1. Püsivara paigaldamine

Avage veebipaigaldaja (link projekti README-s) Chrome'is või Edge'is, ühendage plaat USB-kaabliga ja klõpsake **Install**. Uus seade põleb **lillana**. See käivitab seadistuspaneeli võrgu ise.

## 2. Seadistuspaneeli avamine

1. Hoidke nuppu **BOOT** all 5–20 sekundit ja vabastage. LED muutub **punaseks**. Uuel seadmel on paneel juba sees (lilla LED) – jätke see samm vahele.
2. Ühendage arvuti, millega puldi ühendate, Wi-Fi võrguga `PS5-WAKE-xxxx`, parool `ps5wol-setup`.
3. Avage brauseris **`https://192.168.4.1`** (või `https://ps5-wake-xxxx.local`). Brauser hoiatab, et ühendus pole privaatne, sest ESP32 kasutab oma sertifikaati – valige **Täpsemalt → Jätka** (nimetused sõltuvad brauserist).
4. Valige lehe ülaosas keel.

## 3. Puldi aadresside lugemine

1. Ühendage DualSense USB-kaabliga arvutiga.
2. Laadige paneeli jaotisest **Bluetooth** alla aadressilugeja:
   - **Windows:** `ps5-wake-reader.cmd` – käivitage see topeltklõpsuga. Kui Windows hoiatab tundmatu väljaandja eest, valige **Lisateave → Käivita ikkagi**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Lugeja näitab konsooli ja puldi Bluetoothi aadresse ning kopeerib lõikelauale **seadistuskoodi** (`PS5WAKE:…`). Kleepige see välja **Aadressilugeja seadistuskood** – mõlemad aadressid täidetakse.

Aadressid muutuvad ainult siis, kui seote puldi konsooliga uuesti.

## 4. Seadistamise lõpetamine

1. Valige Wi-Fi võrk nupuga **Otsi**, sisestage parool ja valige riik.
2. Sisestage **Paneeli võrgu uus parool** (vähemalt 8 märki). See on kohustuslik, sest vaikeparool on avalikult teada.
3. Klõpsake **Salvesta ja taaskäivita**. Paneel sulgub, LED vilgub ühendamise ajal **siniselt** ja põleb **roheliselt**, kui kõik on valmis.

## 5. Wake-on-LAN seadistamine

- Kasutage Wake-on-LAN rakenduses (või ruuteri WoL-lehel) paneelil näidatud välja **Selle ESP32 MAC-aadress**, UDP-porti **9** ja võrgu **leviaadressi** (nt `192.168.1.255`).
- Reserveerige ruuteris ESP32 jaoks fikseeritud IP-aadress.
- Kui rakendus või ruuter kasutab juba **PS5 enda võrgu MAC-aadressi**, võite selle sisestada välja **Reageeri ka PS5 enda MAC-aadressile** – ESP32 reageerib siis ka sellele. Mesh-võrkudes (nt ASUS AiMesh) ei pruugi sellised paketid ESP32-ni jõuda; sel juhul saatke WoL ESP32 MAC-aadressile.

## Igapäevane kasutamine

- **Konsooli olek:** paneeli ülaosas toiteikooni juures olev täpp on roheline, kui PS5 on sees, oranž puhkerežiimis ja punane, kui see on väljas või kättesaamatu (hall: veel leidmata). Puhkerežiimi tuvastatakse ainult siis, kui konsool jääb internetiga ühendatuks.
- **PS5 äratamine:** saatke Wake-on-LAN pakett või vajutage lühidalt **BOOT**. LED sähvatab korra valgelt, vilgub katse ajal türkiissinisena ja sähvatab seejärel **3× roheliselt** (konsool vastas) või **3× punaselt** (vastust pole).
- Katsete vahel on vähemalt 30 sekundit; eiratud päringut näitab üks oranž sähvatus.

## Nupp BOOT

| Hoidmise aeg | LED hoidmise ajal | Tulemus |
|---|---|---|
| alla 1 s | – | PS5 äratamine |
| 1–5 s | valge | mitte midagi (tühistamine) |
| 5–20 s, siis vabastamine | punane (vilgub alates 15 s) | seadistuspaneeli avamine |
| 20 s | lilla, vilgub kiiresti | kõigi seadete kustutamine ja taaskäivitus |

Kui hoiate toite ühendamisel BOOT-nuppu all, käivitub turvarežiim ilma Wi-Fi ja Bluetoothita (kollane LED).

## LED-i värvid

| Värv | Käitumine | Tähendus |
|---|---|---|
| lilla | põleb | seadistamata – seadistuspaneel on sees |
| sinine | vilgub aeglaselt | Wi-Fi-ga ühendamine |
| oranž | vilgub aeglaselt | Wi-Fi seadistatud, kuid ühendus puudub üle 30 s |
| roheline (tuhm) | põleb | valmis (saab paneelis välja lülitada) |
| punane | põleb | seadistuspaneel on sees |
| kollane | põleb | turvarežiim |
| valge | 1 sähvatus | Wake-on-LAN vastu võetud või BOOT vajutatud |
| türkiissinine | vilgub kiiresti | äratamiskatse käib |
| roheline | 3 sähvatust | konsool vastas |
| punane | 3 sähvatust | konsool ei vastanud |
| oranž | 1 sähvatus | päring eirati (katse käib või 30 s paus) |

## Seadistuspaneel

- Paneel ja selle võrk lülituvad välja pärast salvestamist, pärast valikut **Sulge muutmata** või 10 minuti tegevusetuse järel. **Pikenda** annab rohkem aega.
- **Lülita LED täielikult välja**: Ei mingeid värve, vilkumist ega sähvatusi. Ainult BOOT-nuppu all hoides näitab LED endiselt hoidmise aega.
- **Seadme nimi (valikuline)**: Nt Elutuba. Kasutatakse paneeli aadressis (`elutuba.local`), Home Assistantis ja lehe pealkirjas – kasulik mitme seadme korral.
- **Juurdepääs koduvõrgust**: Vaikimisi sees. Leht jääb koduvõrgus alati kättesaadavaks aadressidel `<ESP32 IP>` ja `ps5-wake-xxxx.local` – sisselogimine paneeli parooliga (kasutajanimi admin, kui te seda ei muutnud). Paneeli Wi-Fi võrku ei käivitata; selle avamiseks hoidke BOOT-nuppu 5 s.
- **Krüpteeritud ühendus (HTTPS)**: Soovitatav. ESP32 loob oma sertifikaadi, seega hoiatab brauser esimesel külastusel, et ühendus pole privaatne – võrrelge allolevat sõrmejälge ja jätkake. Ilma HTTPS-ita liiguvad parool ja seaded võrgus krüpteerimata. Sertifikaat uuendatakse enne aegumist automaatselt (ESP32 võtab aja internetist); **Uus sertifikaat** loob kohe uue.
- **Ärata PS5 kohe** testib äratamist otse paneelist.

## Tõrkeotsing

- **LED vilgub oranžilt:** vale Wi-Fi parool, ainult 5 GHz võrk või nõrk signaal. Avage paneel, kontrollige Wi-Fi seadeid ja viige ESP32 ruuterile lähemale. ESP32 taaskäivitub ise, kui see on 2 minutit ühenduseta.
- **Pärast Wake-on-LAN saatmist ei juhtu midagi (valget sähvatust pole):** pakett ei jõua ESP32-ni. Kasutage ESP32 MAC-aadressi, leviaadressi ja porti 9; kontrollige, et ESP32 poleks külalisvõrgus.
- **Punased sähvatused pärast katset:** kontrollige Bluetoothi aadresse (lugege need uuesti pärast puldi uuesti sidumist) ja viige ESP32 konsoolile lähemale.
- **Lugeja ei leia pulti:** kasutage andmekaablit (mitte ainult laadimiskaablit) ja ühendage pult otse arvutiga.
- **Brauser hoiatab sertifikaadi eest:** see on ootuspärane – paneel on krüpteeritud ESP32 enda sertifikaadiga. Jätkake lehele; paneel näitab sertifikaadi sõrmejälge.

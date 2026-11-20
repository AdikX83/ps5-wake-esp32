# PS5 Wake – lietotāja rokasgrāmata

PS5 Wake ir neliela ESP32-C6 plate, kas ieslēdz jūsu PlayStation 5, kad saņem Wake-on-LAN paketi vai kad nospiežat tās pogu. Caur Bluetooth tā uzdodas par jūsu savienoto DualSense kontrolieri – tieši tāpat kā PS pogas nospiešana. Darbojas no miega režīma un no pilnībā izslēgtas konsoles.

## Kas nepieciešams

- ESP32-C6 plate ar PS5 Wake (pārbaudīts: ESP32-C6-DevKitM-1), barošana no jebkura USB lādētāja.
- DualSense (vai DualSense Edge) kontrolieris, kas jau savienots pārī ar PS5.
- Dators un USB **datu** kabelis – tikai vienreiz, programmaparatūras instalēšanai un kontroliera adrešu nolasīšanai.
- 2,4 GHz Wi-Fi tīkls.

## 1. Programmaparatūras instalēšana

Atveriet tīmekļa instalētāju (saite projekta README) pārlūkā Chrome vai Edge, pievienojiet plati ar USB kabeli un noklikšķiniet uz **Install**. Jauna ierīce deg **violetā** krāsā. Tā pati palaiž iestatīšanas paneļa tīklu.

## 2. Iestatīšanas paneļa atvēršana

1. Turiet nospiestu pogu **BOOT** 5–20 sekundes un atlaidiet. Gaismas diode iedegas **sarkanā** krāsā. Jaunai ierīcei panelis jau ir ieslēgts (violeta gaismas diode) – izlaidiet šo soli.
2. Datorā, kuram pievienosiet kontrolieri, pievienojieties Wi-Fi tīklam `PS5-WAKE-xxxx`, parole `ps5wol-setup`.
3. Pārlūkā atveriet **`https://192.168.4.1`** (vai `https://ps5-wake-xxxx.local`). Pārlūks brīdinās, ka savienojums nav privāts, jo ESP32 izmanto savu sertifikātu – izvēlieties **Papildu → Turpināt** (nosaukumi atkarīgi no pārlūka).
4. Lapas augšdaļā izvēlieties valodu.

## 3. Kontroliera adrešu nolasīšana

1. Pievienojiet DualSense datoram ar USB kabeli.
2. Paneļa sadaļā **Bluetooth** lejupielādējiet adrešu lasītāju:
   - **Windows:** `ps5-wake-reader.cmd` – palaidiet to ar dubultklikšķi. Ja Windows brīdina par nezināmu izdevēju, izvēlieties **Papildinformācija → Tomēr palaist**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Lasītājs parāda konsoles un kontroliera Bluetooth adreses un starpliktuvē nokopē **iestatīšanas kodu** (`PS5WAKE:…`). Ielīmējiet to laukā **Iestatīšanas kods no adrešu nolasītāja** – abas adreses tiek aizpildītas.

Adreses mainās tikai tad, ja kontrolieri atkārtoti savienojat pārī ar konsoli.

## 4. Iestatīšanas pabeigšana

1. Izvēlieties Wi-Fi tīklu ar pogu **Meklēt**, ievadiet paroli un izvēlieties valsti.
2. Ievadiet **Jauna paneļa tīkla parole** (vismaz 8 rakstzīmes). Tā ir obligāta, jo noklusējuma parole ir publiski zināma.
3. Noklikšķiniet uz **Saglabāt un restartēt**. Panelis aizveras, gaismas diode savienošanas laikā mirgo **zilā** krāsā un deg **zaļā**, kad viss ir gatavs.

## 5. Wake-on-LAN iestatīšana

- Wake-on-LAN lietotnē (vai maršrutētāja WoL lapā) izmantojiet panelī redzamo **Šī ESP32 MAC adrese**, UDP portu **9** un tīkla **apraides adresi** (piem., `192.168.1.255`).
- Maršrutētājā rezervējiet ESP32 fiksētu IP adresi.
- Ja lietotne vai maršrutētājs jau izmanto **pašas PS5 tīkla MAC adresi**, varat to ievadīt laukā **Reaģēt arī uz pašas PS5 MAC adresi** – ESP32 reaģēs arī uz to. Mesh tīklos (piem., ASUS AiMesh) šādas paketes var nesasniegt ESP32; tad sūtiet WoL uz ESP32 MAC adresi.

## Ikdienas lietošana

- **Konsoles stāvoklis:** punkts pie barošanas ikonas paneļa augšdaļā ir zaļš, kad PS5 ir ieslēgta, oranžs miega režīmā un sarkans, kad tā ir izslēgta vai nepieejama (pelēks: vēl nav atrasta). Miega režīms tiek noteikts tikai tad, ja konsole paliek savienota ar internetu.
- **PS5 pamodināšana:** nosūtiet Wake-on-LAN paketi vai īsi nospiediet **BOOT**. Gaismas diode vienreiz uzzibsnī baltā krāsā, mēģinājuma laikā mirgo tirkīzā un pēc tam uzzibsnī **3× zaļā** (konsole atbildēja) vai **3× sarkanā** (nav atbildes).
- Starp mēģinājumiem paiet vismaz 30 sekundes; ignorētu pieprasījumu norāda viens oranžs zibsnis.

## Poga BOOT

| Turēšanas laiks | Gaismas diode turot | Rezultāts |
|---|---|---|
| mazāk par 1 s | – | PS5 pamodināšana |
| 1–5 s | balta | nekas (atcelšana) |
| 5–20 s, tad atlaist | sarkana (no 15 s mirgo) | iestatīšanas paneļa atvēršana |
| 20 s | violeta, ātri mirgo | visu iestatījumu dzēšana un restartēšana |

Ja, pievienojot barošanu, turat nospiestu BOOT, startē drošais režīms bez Wi-Fi un Bluetooth (dzeltena gaismas diode).

## Gaismas diodes krāsas

| Krāsa | Darbība | Nozīme |
|---|---|---|
| violeta | deg | nav iestatīts – iestatīšanas panelis ir ieslēgts |
| zila | lēni mirgo | savienojas ar Wi-Fi |
| oranža | lēni mirgo | Wi-Fi iestatīts, bet nav savienojuma ilgāk par 30 s |
| zaļa (blāva) | deg | gatavs (var izslēgt panelī) |
| sarkana | deg | iestatīšanas panelis ir ieslēgts |
| dzeltena | deg | drošais režīms |
| balta | 1 zibsnis | saņemts Wake-on-LAN vai nospiests BOOT |
| tirkīza | ātri mirgo | notiek pamodināšanas mēģinājums |
| zaļa | 3 zibšņi | konsole atbildēja |
| sarkana | 3 zibšņi | konsole neatbildēja |
| oranža | 1 zibsnis | pieprasījums ignorēts (notiek mēģinājums vai 30 s pauze) |

## Iestatīšanas panelis

- Panelis un tā tīkls izslēdzas pēc saglabāšanas, pēc **Aizvērt bez izmaiņām** vai pēc 10 minūtēm bez darbības. **Pagarināt** dod vairāk laika.
- **Pilnībā izslēgt gaismas diodi**: Nekādu krāsu, mirgošanas vai zibšņu. Tikai turot BOOT, gaismas diode joprojām rāda turēšanas laiku.
- **Ierīces nosaukums (neobligāts)**: Piem., Viesistaba. Tiek izmantots paneļa adresē (`viesistaba.local`), Home Assistant un lapas virsrakstā – noderīgi, ja ierīces ir vairākas.
- **Piekļuve no mājas tīkla**: Pēc noklusējuma ieslēgts. Lapa paliek pastāvīgi pieejama mājas tīklā adresē `<ESP32 IP>` un `ps5-wake-xxxx.local` – pieteikšanās ar paneļa paroli (lietotājvārds admin, ja neesat to mainījis). Paneļa Wi-Fi tīkls netiek palaists; lai to atvērtu, turiet BOOT 5 s.
- **Šifrēts savienojums (HTTPS)**: Ieteicams. ESP32 izveido savu sertifikātu, tāpēc pirmajā apmeklējumā pārlūks brīdina, ka savienojums nav privāts – salīdziniet zemāk redzamo nospiedumu un turpiniet. Bez HTTPS parole un iestatījumi tīklā tiek pārsūtīti nešifrēti. Sertifikāts tiek automātiski atjaunots pirms derīguma beigām (ESP32 iegūst laiku no interneta); **Jauns sertifikāts** uzreiz izveido jaunu.
- **Pamodināt PS5 tagad** pārbauda pamodināšanu tieši no paneļa.

## Problēmu novēršana

- **Gaismas diode mirgo oranžā krāsā:** nepareiza Wi-Fi parole, tikai 5 GHz tīkls vai vājš signāls. Atveriet paneli, pārbaudiet Wi-Fi un novietojiet ESP32 tuvāk maršrutētājam. ESP32 pats restartējas, ja 2 minūtes nav savienojuma.
- **Pēc Wake-on-LAN nosūtīšanas nekas nenotiek (nav balta zibšņa):** pakete nesasniedz ESP32. Izmantojiet ESP32 MAC adresi, apraides adresi un portu 9; pārbaudiet, vai ESP32 nav viesu tīklā.
- **Sarkani zibšņi pēc mēģinājuma:** pārbaudiet Bluetooth adreses (nolasiet tās vēlreiz pēc kontroliera atkārtotas savienošanas pārī) un novietojiet ESP32 tuvāk konsolei.
- **Lasītājs neatrod kontrolieri:** izmantojiet datu kabeli (ne tikai uzlādes) un pievienojiet kontrolieri tieši datoram.
- **Pārlūks brīdina par sertifikātu:** tas ir paredzēts – panelis ir šifrēts ar ESP32 paša sertifikātu. Turpiniet uz lapu; panelis rāda sertifikāta nospiedumu.

# PS5 Wake – naudotojo vadovas

PS5 Wake – tai maža ESP32-C6 plokštė, kuri įjungia jūsų PlayStation 5, kai gauna Wake-on-LAN paketą arba kai paspaudžiate jos mygtuką. Per Bluetooth ji apsimeta jūsų susietu DualSense valdikliu – lygiai kaip PS mygtuko paspaudimas. Veikia iš poilsio režimo ir visiškai išjungus konsolę.

## Ko reikia

- ESP32-C6 plokštė su PS5 Wake (išbandyta: ESP32-C6-DevKitM-1), maitinama iš bet kurio USB įkroviklio.
- DualSense (arba DualSense Edge) valdiklis, jau susietas su PS5.
- Kompiuteris ir USB **duomenų** kabelis – tik kartą, programinei aparatinei įrangai įdiegti ir valdiklio adresams nuskaityti.
- 2,4 GHz Wi-Fi tinklas.

## 1. Programinės aparatinės įrangos įdiegimas

Atidarykite žiniatinklio diegimo programą (nuoroda projekto README) naršyklėje Chrome arba Edge, prijunkite plokštę USB kabeliu ir spustelėkite **Install**. Naujas įrenginys šviečia **violetine** spalva. Jis pats paleidžia nustatymų skydelio tinklą.

## 2. Nustatymų skydelio atidarymas

1. Laikykite nuspaustą mygtuką **BOOT** 5–20 sekundžių ir atleiskite. Šviesos diodas užsidega **raudonai**. Naujame įrenginyje skydelis jau įjungtas (violetinis šviesos diodas) – praleiskite šį žingsnį.
2. Kompiuteryje, prie kurio prijungsite valdiklį, prisijunkite prie Wi-Fi tinklo `PS5-WAKE-xxxx`, slaptažodis `ps5wol-setup`.
3. Naršyklėje atidarykite **`https://192.168.4.1`** (arba `https://ps5-wake-xxxx.local`). Naršyklė įspės, kad ryšys nėra privatus, nes ESP32 naudoja savo sertifikatą – pasirinkite **Išplėstiniai → Tęsti** (pavadinimai priklauso nuo naršyklės).
4. Puslapio viršuje pasirinkite kalbą.

## 3. Valdiklio adresų nuskaitymas

1. Prijunkite DualSense prie kompiuterio USB kabeliu.
2. Skydelio skyriuje **„Bluetooth“** atsisiųskite adresų skaitytuvą:
   - **Windows:** `ps5-wake-reader.cmd` – paleiskite jį dukart spustelėdami. Jei Windows įspėja apie nežinomą leidėją, pasirinkite **Daugiau informacijos → Vis tiek vykdyti**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Skaitytuvas parodo konsolės ir valdiklio Bluetooth adresus ir nukopijuoja į mainų sritį **nustatymo kodą** (`PS5WAKE:…`). Įklijuokite jį į lauką **Sąrankos kodas iš adresų skaitytuvo** – abu adresai užpildomi.

Adresai keičiasi tik tada, kai valdiklį su konsole susiejate iš naujo.

## 4. Nustatymo užbaigimas

1. Pasirinkite Wi-Fi tinklą mygtuku **Ieškoti**, įveskite slaptažodį ir pasirinkite šalį.
2. Įveskite **Naujas skydelio tinklo slaptažodis** (bent 8 simboliai). Jis privalomas, nes numatytasis slaptažodis yra viešai žinomas.
3. Spustelėkite **Išsaugoti ir paleisti iš naujo**. Skydelis užsidaro, šviesos diodas jungiantis mirksi **mėlynai** ir šviečia **žaliai**, kai viskas paruošta.

## 5. Wake-on-LAN nustatymas

- Wake-on-LAN programėlėje (arba maršruto parinktuvo WoL puslapyje) naudokite skydelyje rodomą **Šio ESP32 MAC adresas**, UDP prievadą **9** ir tinklo **transliavimo adresą** (pvz., `192.168.1.255`).
- Maršruto parinktuve rezervuokite ESP32 fiksuotą IP adresą.
- Jei programėlė ar maršruto parinktuvas jau naudoja **pačios PS5 tinklo MAC adresą**, galite jį įvesti lauke **Reaguoti ir į pačios PS5 MAC adresą** – ESP32 reaguos ir į jį. „Mesh“ tinkluose (pvz., ASUS AiMesh) tokie paketai gali nepasiekti ESP32; tada siųskite WoL ESP32 MAC adresu.

## Kasdienis naudojimas

- **Konsolės būsena:** taškas prie maitinimo piktogramos skydelio viršuje yra žalias, kai PS5 įjungta, oranžinis poilsio režime ir raudonas, kai ji išjungta arba nepasiekiama (pilkas: dar nerasta). Poilsio režimas aptinkamas tik tada, kai konsolė lieka prisijungusi prie interneto.
- **PS5 pažadinimas:** išsiųskite Wake-on-LAN paketą arba trumpai paspauskite **BOOT**. Šviesos diodas kartą sublyksi baltai, bandymo metu mirksi žalsvai mėlynai, o tada sublyksi **3× žaliai** (konsolė atsakė) arba **3× raudonai** (atsakymo nėra).
- Tarp bandymų praeina bent 30 sekundžių; ignoruotą užklausą rodo vienas oranžinis blyksnis.

## Mygtukas BOOT

| Laikymo trukmė | Šviesos diodas laikant | Rezultatas |
|---|---|---|
| mažiau nei 1 s | – | PS5 pažadinimas |
| 1–5 s | baltas | nieko (atšaukimas) |
| 5–20 s, tada atleisti | raudonas (nuo 15 s mirksi) | nustatymų skydelio atidarymas |
| 20 s | violetinis, greitai mirksi | visų nustatymų ištrynimas ir paleidimas iš naujo |

Jei jungdami maitinimą laikote nuspaustą BOOT, paleidžiamas saugusis režimas be Wi-Fi ir Bluetooth (geltonas šviesos diodas).

## Šviesos diodo spalvos

| Spalva | Elgsena | Reikšmė |
|---|---|---|
| violetinė | šviečia | nenustatyta – nustatymų skydelis įjungtas |
| mėlyna | lėtai mirksi | jungiamasi prie Wi-Fi |
| oranžinė | lėtai mirksi | Wi-Fi nustatytas, bet ryšio nėra ilgiau nei 30 s |
| žalia (pritemdyta) | šviečia | paruošta (galima išjungti skydelyje) |
| raudona | šviečia | nustatymų skydelis įjungtas |
| geltona | šviečia | saugusis režimas |
| balta | 1 blyksnis | gautas Wake-on-LAN arba paspaustas BOOT |
| žalsvai mėlyna | greitai mirksi | vyksta žadinimo bandymas |
| žalia | 3 blyksniai | konsolė atsakė |
| raudona | 3 blyksniai | konsolė neatsakė |
| oranžinė | 1 blyksnis | užklausa ignoruota (vyksta bandymas arba 30 s pertrauka) |

## Nustatymų skydelis

- Skydelis ir jo tinklas išsijungia išsaugojus, paspaudus **Uždaryti be pakeitimų** arba po 10 minučių neveiklumo. **Pratęsti** suteikia daugiau laiko.
- **Visiškai išjungti šviesos diodą**: Jokių spalvų, mirksėjimo ar blyksnių. Tik laikant BOOT šviesos diodas vis dar rodo laikymo trukmę.
- **Įrenginio pavadinimas (neprivaloma)**: Pvz., Svetainė. Naudojamas skydelio adrese (`svetaine.local`), Home Assistant ir puslapio pavadinime – naudinga, kai įrenginių keli.
- **Prieiga iš namų tinklo**: Įjungta pagal numatytuosius nustatymus. Puslapis nuolat pasiekiamas namų tinkle adresu `<ESP32 IP>` ir `ps5-wake-xxxx.local` – prisijunkite skydelio slaptažodžiu (naudotojo vardas admin, jei jo nepakeitėte). Skydelio Wi-Fi tinklas nepaleidžiamas; norėdami jį atidaryti, 5 s laikykite BOOT.
- **Šifruotas ryšys (HTTPS)**: Rekomenduojama. ESP32 susikuria savo sertifikatą, todėl pirmą kartą apsilankius naršyklė įspės, kad ryšys nėra privatus – palyginkite toliau pateiktą kontrolinį kodą ir tęskite. Be HTTPS slaptažodis ir nustatymai tinkle perduodami nešifruoti. Sertifikatas automatiškai atnaujinamas prieš baigiantis galiojimui (ESP32 laiką gauna iš interneto); **Naujas sertifikatas** iškart sukuria naują.
- **Pažadinti PS5 dabar** išbando žadinimą tiesiai iš skydelio.

## Trikčių šalinimas

- **Šviesos diodas mirksi oranžine spalva:** neteisingas Wi-Fi slaptažodis, tik 5 GHz tinklas arba silpnas signalas. Atidarykite skydelį, patikrinkite Wi-Fi ir perkelkite ESP32 arčiau maršruto parinktuvo. ESP32 pats pasileidžia iš naujo, jei 2 minutes neturi ryšio.
- **Išsiuntus Wake-on-LAN nieko nevyksta (nėra balto blyksnio):** paketas nepasiekia ESP32. Naudokite ESP32 MAC adresą, transliavimo adresą ir prievadą 9; patikrinkite, ar ESP32 nėra svečių tinkle.
- **Raudoni blyksniai po bandymo:** patikrinkite Bluetooth adresus (iš naujo susieję valdiklį, nuskaitykite juos dar kartą) ir perkelkite ESP32 arčiau konsolės.
- **Skaitytuvas neranda valdiklio:** naudokite duomenų kabelį (ne tik įkrovimo) ir prijunkite valdiklį tiesiai prie kompiuterio.
- **Naršyklė įspėja dėl sertifikato:** tai normalu – skydelis šifruojamas paties ESP32 sertifikatu. Tęskite į puslapį; skydelis rodo sertifikato kontrolinį kodą.

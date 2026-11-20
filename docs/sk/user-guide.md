# PS5 Wake – používateľská príručka

PS5 Wake je malá doska ESP32-C6, ktorá zapne váš PlayStation 5, keď prijme paket Wake-on-LAN alebo keď stlačíte jej tlačidlo. Cez Bluetooth sa vydáva za váš spárovaný ovládač DualSense – rovnako ako stlačenie tlačidla PS. Funguje z režimu odpočinku aj z úplne vypnutej konzoly.

## Čo budete potrebovať

- Dosku ESP32-C6 s PS5 Wake (testované: ESP32-C6-DevKitM-1), napájanú z ľubovoľnej USB nabíjačky.
- Ovládač DualSense (alebo DualSense Edge) už spárovaný s vašou PS5.
- Počítač a **dátový** USB kábel – len raz, na inštaláciu firmvéru a načítanie adries ovládača.
- Sieť Wi-Fi 2,4 GHz.

## 1. Inštalácia firmvéru

Otvorte webový inštalátor (odkaz v README projektu) v Chrome alebo Edge, pripojte dosku USB káblom a kliknite na **Install or update**. Nové zariadenie svieti **fialovo**. Sám spustí sieť konfiguračného panela.

## 2. Otvorenie konfiguračného panela

1. Podržte tlačidlo **BOOT** 5 – 20 sekúnd a pustite ho. LED sa rozsvieti **červeno**. Pri novom zariadení je panel zapnutý hneď (fialová LED) – tento krok preskočte.
2. Na počítači, ku ktorému pripojíte ovládač, sa pripojte k sieti Wi-Fi `PS5-WAKE-xxxx`, heslo `ps5wol-setup`.
3. V prehliadači otvorte **`https://192.168.4.1`** (alebo `https://ps5-wake-xxxx.local`). Prehliadač upozorní, že pripojenie nie je súkromné, pretože ESP32 používa vlastný certifikát – zvoľte **Rozšírené → Pokračovať** (názvy sa líšia podľa prehliadača).
4. Hore na stránke zvoľte jazyk.

## 3. Načítanie adries ovládača

1. Pripojte DualSense k počítaču USB káblom.
2. V časti **Bluetooth** panela stiahnite čítačku adries:
   - **Windows:** `ps5-wake-reader.cmd` – spustite dvojklikom. Ak Windows varuje pred neznámym vydavateľom, zvoľte **Ďalšie informácie → Spustiť aj tak**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Čítačka zobrazí Bluetooth adresy konzoly a ovládača a skopíruje do schránky **konfiguračný kód** (`PS5WAKE:…`). Vložte ho do poľa **Konfiguračný kód z čítačky adries** – obe adresy sa vyplnia.

Adresy sa menia len pri novom spárovaní ovládača s konzolou.

## 4. Dokončenie nastavenia

1. Vyberte svoju sieť Wi-Fi tlačidlom **Hľadať**, zadajte heslo a zvoľte krajinu.
2. Zadajte **Nové heslo siete panela** (aspoň 8 znakov). Je povinné, pretože predvolené heslo je verejne známe.
3. Kliknite na **Uložiť a reštartovať**. Panel sa zatvorí, LED pri pripájaní bliká **modro** a keď je všetko pripravené, svieti **zeleno**.

## 5. Nastavenie Wake-on-LAN

- V aplikácii Wake-on-LAN (alebo na stránke WoL routera) použite **MAC adresa tohto ESP32** z panela, UDP port **9** a **broadcast adresu** siete (napr. `192.168.1.255`).
- Rezervujte v routeri pre ESP32 pevnú IP adresu.
- Ak aplikácia alebo router už používajú **sieťovú MAC adresu samotnej PS5**, môžete ju zadať do poľa **Reagovať aj na MAC adresu samotnej PS5** – ESP32 potom reaguje aj na ňu. V sieťach mesh (napr. ASUS AiMesh) takéto pakety nemusia k ESP32 doraziť; vtedy posielajte WoL na MAC adresu ESP32.

## Každodenné používanie

- **Stav konzoly:** bodka pri ikone napájania hore v paneli je zelená, keď je PS5 zapnutá, oranžová v režime odpočinku a červená, keď je vypnutá alebo nedostupná (sivá: zatiaľ nenájdená). Režim odpočinku sa rozpozná, len ak konzola zostáva pripojená na internet.
- **Prebudenie PS5:** pošlite paket Wake-on-LAN alebo krátko stlačte **BOOT**. LED blikne nabielo, počas pokusu bliká tyrkysovo a potom blikne **3× nazeleno** (konzola odpovedala) alebo **3× načerveno** (bez odpovede).
- Medzi pokusmi je aspoň 30 sekúnd; ignorovanú požiadavku signalizuje jedno oranžové bliknutie.

## Tlačidlo BOOT

| Doba držania | LED počas držania | Výsledok |
|---|---|---|
| menej ako 1 s | – | prebudenie PS5 |
| 1 – 5 s | biela | nič (zrušenie) |
| 5 – 20 s, potom pustiť | červená (od 15 s bliká) | otvorenie konfiguračného panela |
| 20 s | fialová, rýchlo bliká | vymazanie všetkých nastavení a reštart |

Držanie BOOT pri pripájaní napájania spustí núdzový režim bez Wi-Fi a Bluetooth (žltá LED).

## Farby LED

| Farba | Správanie | Význam |
|---|---|---|
| fialová | svieti | nenastavené – konfiguračný panel je zapnutý |
| modrá | pomaly bliká | pripájanie k Wi-Fi |
| oranžová | pomaly bliká | Wi-Fi nastavené, ale dlhšie ako 30 s nepripojené |
| zelená (stlmená) | svieti | pripravené (dá sa vypnúť v paneli) |
| červená | svieti | konfiguračný panel je zapnutý |
| žltá | svieti | núdzový režim |
| biela | 1 bliknutie | prijatý Wake-on-LAN alebo stlačené BOOT |
| tyrkysová | rýchlo bliká | prebieha pokus o prebudenie |
| zelená | 3 bliknutia | konzola odpovedala |
| červená | 3 bliknutia | konzola neodpovedala |
| oranžová | 1 bliknutie | požiadavka ignorovaná (prebieha pokus alebo 30 s pauza) |

## Konfiguračný panel

- Panel a jeho sieť sa vypnú po uložení, po **Zavrieť bez zmien** alebo po 10 minútach bez činnosti. **Predĺžiť** pridá čas.
- **Úplne vypnúť LED**: Žiadne farby, blikanie ani záblesky. Len počas držania BOOT LED naďalej ukazuje dobu držania.
- **Názov zariadenia (voliteľné)**: Napr. Obývačka. Používa sa v adrese panela (`obyvacka.local`), v Home Assistant a v názve stránky – užitočné pri viacerých zariadeniach.
- **Prístup z domácej siete**: Predvolene zapnuté. Stránka zostane trvalo dostupná v domácej sieti na adrese `<IP ESP32>` a `ps5-wake-xxxx.local` – prihlásenie heslom panela (používateľské meno admin, ak ste ho nezmenili). Sieť Wi-Fi panela sa nespúšťa; na jej otvorenie podržte BOOT 5 s.
- **Šifrované pripojenie (HTTPS)**: Odporúčané. ESP32 si vytvára vlastný certifikát, takže pri prvej návšteve prehliadač upozorní, že pripojenie nie je súkromné – porovnajte odtlačok nižšie a pokračujte. Bez HTTPS sa heslo a nastavenia prenášajú sieťou nešifrovane. Certifikát sa pred uplynutím obnovuje automaticky (ESP32 si berie čas z internetu); **Nový certifikát** vytvorí nový okamžite.
- **Prebudiť PS5 teraz** otestuje prebudenie priamo z panela.

## Riešenie problémov

- **LED bliká oranžovo:** zlé heslo Wi-Fi, sieť len 5 GHz alebo slabý signál. Otvorte panel, skontrolujte Wi-Fi a presuňte ESP32 bližšie k routeru. ESP32 sa sám reštartuje, ak je 2 minúty offline.
- **Po odoslaní Wake-on-LAN sa nič nedeje (žiadne biele bliknutie):** paket k ESP32 nedorazí. Použite MAC adresu ESP32, broadcast adresu a port 9; overte, že ESP32 nie je v sieti pre hostí.
- **Červené bliknutia po pokuse:** skontrolujte Bluetooth adresy (po novom spárovaní ovládača ich načítajte znova) a presuňte ESP32 bližšie ku konzole.
- **Čítačka nenájde ovládač:** použite dátový kábel (nie len nabíjací) a pripojte ovládač priamo k počítaču.
- **Prehliadač varuje pred certifikátom:** je to v poriadku – panel je šifrovaný vlastným certifikátom ESP32. Pokračujte na stránku; panel zobrazuje odtlačok certifikátu.

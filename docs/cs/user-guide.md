# PS5 Wake – uživatelská příručka

PS5 Wake je malá deska ESP32-C6, která zapne váš PlayStation 5, když přijme paket Wake-on-LAN nebo když stisknete její tlačítko. Přes Bluetooth se vydává za váš spárovaný ovladač DualSense – stejně jako stisk tlačítka PS. Funguje z klidového režimu i z úplně vypnuté konzole.

## Co budete potřebovat

- Desku ESP32-C6 s PS5 Wake (testováno: ESP32-C6-DevKitM-1), napájenou z libovolné USB nabíječky.
- Ovladač DualSense (nebo DualSense Edge) již spárovaný s vaší PS5.
- Počítač a **datový** USB kabel – jen jednou, pro instalaci firmwaru a načtení adres ovladače.
- Síť Wi-Fi 2,4 GHz.

## 1. Instalace firmwaru

Otevřete webový instalátor (odkaz v README projektu) v Chrome nebo Edge, připojte desku USB kabelem a klikněte na **Install**. Nové zařízení svítí **fialově**. Sám spustí síť konfiguračního panelu.

## 2. Otevření konfiguračního panelu

1. Podržte tlačítko **BOOT** 5–20 sekund a pusťte ho. LED se rozsvítí **červeně**. U nového zařízení je panel zapnutý hned (fialová LED) – tento krok přeskočte.
2. Na počítači, ke kterému připojíte ovladač, se připojte k síti Wi-Fi `PS5-WAKE-xxxx`, heslo `ps5wol-setup`.
3. V prohlížeči otevřete **`https://192.168.4.1`** (nebo `https://ps5-wake-xxxx.local`). Prohlížeč upozorní, že připojení není soukromé, protože ESP32 používá vlastní certifikát – zvolte **Rozšířená nastavení → Pokračovat** (názvy se liší podle prohlížeče).
4. Nahoře na stránce zvolte jazyk.

## 3. Načtení adres ovladače

1. Připojte DualSense k počítači USB kabelem.
2. V části **Bluetooth** panelu stáhněte čtečku adres:
   - **Windows:** `ps5-wake-reader.cmd` – spusťte dvojklikem. Pokud Windows varuje před neznámým vydavatelem, zvolte **Další informace → Přesto spustit**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Čtečka zobrazí Bluetooth adresy konzole a ovladače a zkopíruje do schránky **konfigurační kód** (`PS5WAKE:…`). Vložte ho do pole **Konfigurační kód ze čtečky adres** – obě adresy se vyplní.

Adresy se mění jen při novém spárování ovladače s konzolí.

## 4. Dokončení nastavení

1. Vyberte svou síť Wi-Fi tlačítkem **Hledat**, zadejte heslo a zvolte zemi.
2. Zadejte **Nové heslo sítě panelu** (alespoň 8 znaků). Je povinné, protože výchozí heslo je veřejně známé.
3. Klikněte na **Uložit a restartovat**. Panel se zavře, LED při připojování bliká **modře** a když je vše připraveno, svítí **zeleně**.

## 5. Nastavení Wake-on-LAN

- V aplikaci Wake-on-LAN (nebo na stránce WoL routeru) použijte **MAC adresa tohoto ESP32** z panelu, UDP port **9** a **broadcast adresu** sítě (např. `192.168.1.255`).
- Rezervujte v routeru pro ESP32 pevnou IP adresu.
- Pokud aplikace nebo router již používají **síťovou MAC adresu samotné PS5**, můžete ji zadat do pole **Reagovat také na MAC adresu samotné PS5** – ESP32 pak reaguje i na ni. V sítích mesh (např. ASUS AiMesh) takové pakety nemusí k ESP32 dorazit; pak posílejte WoL na MAC adresu ESP32.

## Každodenní použití

- **Stav konzole:** tečka u ikony napájení nahoře v panelu je zelená, když je PS5 zapnutá, oranžová v klidovém režimu a červená, když je vypnutá nebo nedostupná (šedá: zatím nenalezena). Klidový režim se rozpozná jen tehdy, když konzole zůstává připojená k internetu.
- **Probuzení PS5:** pošlete paket Wake-on-LAN nebo krátce stiskněte **BOOT**. LED blikne bíle, během pokusu bliká tyrkysově a pak blikne **3× zeleně** (konzole odpověděla) nebo **3× červeně** (bez odpovědi).
- Mezi pokusy je alespoň 30 sekund; ignorovaný požadavek signalizuje jedno oranžové bliknutí.

## Tlačítko BOOT

| Doba držení | LED při držení | Výsledek |
|---|---|---|
| méně než 1 s | – | probuzení PS5 |
| 1–5 s | bílá | nic (zrušení) |
| 5–20 s, pak pustit | červená (od 15 s bliká) | otevření konfiguračního panelu |
| 20 s | fialová, rychle bliká | smazání všech nastavení a restart |

Držení BOOT při připojování napájení spustí nouzový režim bez Wi-Fi a Bluetooth (žlutá LED).

## Barvy LED

| Barva | Chování | Význam |
|---|---|---|
| fialová | svítí | nenastaveno – konfigurační panel je zapnutý |
| modrá | pomalu bliká | připojování k Wi-Fi |
| oranžová | pomalu bliká | Wi-Fi nastaveno, ale déle než 30 s nepřipojeno |
| zelená (ztlumená) | svítí | připraveno (lze vypnout v panelu) |
| červená | svítí | konfigurační panel je zapnutý |
| žlutá | svítí | nouzový režim |
| bílá | 1 bliknutí | přijat Wake-on-LAN nebo stisknuto BOOT |
| tyrkysová | rychle bliká | probíhá pokus o probuzení |
| zelená | 3 bliknutí | konzole odpověděla |
| červená | 3 bliknutí | konzole neodpověděla |
| oranžová | 1 bliknutí | požadavek ignorován (probíhá pokus nebo 30 s pauza) |

## Konfigurační panel

- Panel a jeho síť se vypnou po uložení, po **Zavřít bez změn** nebo po 10 minutách bez činnosti. **Prodloužit** přidá čas.
- **Úplně vypnout LED**: Žádné barvy, blikání ani záblesky. Jen při držení BOOT LED dál ukazuje dobu držení.
- **Název zařízení (volitelné)**: Např. Obývák. Použije se v adrese panelu (`obyvak.local`), v Home Assistant a v titulku stránky – hodí se při více zařízeních.
- **Přístup z domácí sítě**: Ve výchozím stavu zapnuto. Stránka zůstane trvale dostupná v domácí síti na adrese `<IP ESP32>` a `ps5-wake-xxxx.local` – přihlášení heslem panelu (uživatelské jméno admin, pokud jste ho nezměnili). Síť Wi-Fi panelu se nespouští; pro její otevření podržte BOOT 5 s.
- **Šifrované připojení (HTTPS)**: Doporučeno. ESP32 si vytváří vlastní certifikát, takže při první návštěvě prohlížeč upozorní, že připojení není soukromé – porovnejte otisk níže a pokračujte. Bez HTTPS se heslo a nastavení přenášejí sítí nešifrovaně. Certifikát se před vypršením obnovuje automaticky (ESP32 si bere čas z internetu); **Nový certifikát** vytvoří nový ihned.
- **Probudit PS5 teď** otestuje probuzení přímo z panelu.

## Řešení problémů

- **LED bliká oranžově:** špatné heslo Wi-Fi, síť jen 5 GHz nebo slabý signál. Otevřete panel, zkontrolujte Wi-Fi a přesuňte ESP32 blíž k routeru. ESP32 se sám restartuje, pokud je 2 minuty offline.
- **Po odeslání Wake-on-LAN se nic neděje (žádné bílé bliknutí):** paket k ESP32 nedorazí. Použijte MAC adresu ESP32, broadcast adresu a port 9; ověřte, že ESP32 není v síti pro hosty.
- **Červená bliknutí po pokusu:** zkontrolujte Bluetooth adresy (po novém spárování ovladače je načtěte znovu) a přesuňte ESP32 blíž ke konzoli.
- **Čtečka nenajde ovladač:** použijte datový kabel (ne jen nabíjecí) a připojte ovladač přímo k počítači.
- **Prohlížeč varuje před certifikátem:** to je v pořádku – panel je šifrován vlastním certifikátem ESP32. Pokračujte na stránku; panel zobrazuje otisk certifikátu.

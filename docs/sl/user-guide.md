# PS5 Wake – navodila za uporabo

PS5 Wake je majhna plošča ESP32-C6, ki vklopi vaš PlayStation 5, ko prejme paket Wake-on-LAN ali ko pritisnete njen gumb. Prek Bluetootha se predstavi kot vaš seznanjeni krmilnik DualSense – enako kot pritisk na gumb PS. Deluje iz načina mirovanja in pri popolnoma izklopljeni konzoli.

## Kaj potrebujete

- Ploščo ESP32-C6 s PS5 Wake (preizkušeno: ESP32-C6-DevKitM-1), napajano iz poljubnega polnilnika USB.
- Krmilnik DualSense (ali DualSense Edge), ki je že seznanjen s PS5.
- Računalnik in **podatkovni** kabel USB – samo enkrat, za namestitev vdelane programske opreme in branje naslovov krmilnika.
- Omrežje Wi-Fi 2,4 GHz.

## 1. Namestitev vdelane programske opreme

V brskalniku Chrome ali Edge odprite spletni namestitveni program (povezava v README projekta), ploščo povežite s kablom USB in kliknite **Install**. Nova naprava sveti **vijolično**. Samodejno zažene omrežje nastavitvene plošče.

## 2. Odpiranje nastavitvene plošče

1. Gumb **BOOT** držite 5–20 sekund in ga spustite. LED zasveti **rdeče**. Pri novi napravi je plošča že vklopljena (vijolična LED) – ta korak preskočite.
2. Na računalniku, na katerega boste priključili krmilnik, se povežite z omrežjem Wi-Fi `PS5-WAKE-xxxx`, geslo `ps5wol-setup`.
3. V brskalniku odprite **`https://192.168.4.1`** (ali `https://ps5-wake-xxxx.local`). Brskalnik opozori, da povezava ni zasebna, ker ESP32 uporablja lastno potrdilo – izberite **Dodatno → Nadaljuj** (imena so odvisna od brskalnika).
4. Na vrhu strani izberite jezik.

## 3. Branje naslovov krmilnika

1. Krmilnik DualSense s kablom USB priključite na računalnik.
2. V razdelku **Bluetooth** prenesite bralnik naslovov:
   - **Windows:** `ps5-wake-reader.cmd` – zaženite ga z dvojnim klikom. Če Windows opozori na neznanega izdajatelja, izberite **Več informacij → Vseeno zaženi**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Bralnik prikaže naslova Bluetooth konzole in krmilnika ter v odložišče kopira **nastavitveno kodo** (`PS5WAKE:…`). Prilepite jo v polje **Nastavitvena koda iz bralnika naslovov** – oba naslova se izpolnita.

Naslova se spremenita le, če krmilnik znova seznanite s konzolo.

## 4. Dokončanje nastavitve

1. Z gumbom **Išči** izberite omrežje Wi-Fi, vnesite geslo in izberite državo.
2. Vnesite **Novo geslo omrežja plošče** (vsaj 8 znakov). Obvezno je, ker je privzeto geslo javno znano.
3. Kliknite **Shrani in znova zaženi**. Plošča se zapre, LED med povezovanjem utripa **modro** in zasveti **zeleno**, ko je vse pripravljeno.

## 5. Nastavitev Wake-on-LAN

- V aplikaciji Wake-on-LAN (ali na strani WoL usmerjevalnika) uporabite **Naslov MAC te naprave ESP32** s plošče, vrata UDP **9** in **naslov za oddajanje** omrežja (npr. `192.168.1.255`).
- V usmerjevalniku rezervirajte stalen naslov IP za ESP32.
- Če aplikacija ali usmerjevalnik že uporablja **omrežni MAC same PS5**, ga lahko vnesete v polje **Odzovi se tudi na MAC same konzole PS5** – ESP32 se bo odzval tudi nanj. V omrežjih mesh (npr. ASUS AiMesh) takšni paketi morda ne pridejo do ESP32; takrat pošiljajte WoL na MAC naprave ESP32.

## Vsakodnevna uporaba

- **Stanje konzole:** pika ob ikoni za vklop na vrhu plošče je zelena, ko je PS5 vklopljena, oranžna v načinu mirovanja in rdeča, ko je izklopljena ali nedosegljiva (siva: še ni najdena). Način mirovanja je zaznan le, če konzola ostane povezana z internetom.
- **Zbujanje PS5:** pošljite paket Wake-on-LAN ali na kratko pritisnite **BOOT**. LED enkrat belo utripne, med poskusom utripa turkizno, nato pa utripne **3× zeleno** (konzola se je odzvala) ali **3× rdeče** (ni odziva).
- Med poskusi mine vsaj 30 sekund; prezrta zahteva enkrat oranžno utripne.

## Gumb BOOT

| Čas držanja | LED med držanjem | Rezultat |
|---|---|---|
| manj kot 1 s | – | zbujanje PS5 |
| 1–5 s | bela | nič (preklic) |
| 5–20 s, nato spustite | rdeča (utripa od 15 s) | odpiranje nastavitvene plošče |
| 20 s | vijolična, hitro utripa | izbris vseh nastavitev in ponovni zagon |

Če med priklopom napajanja držite BOOT, se zažene varni način brez Wi-Fi in Bluetootha (rumena LED).

## Barve LED

| Barva | Vedenje | Pomen |
|---|---|---|
| vijolična | sveti | ni nastavljeno – nastavitvena plošča je vklopljena |
| modra | počasi utripa | povezovanje z Wi-Fi |
| oranžna | počasi utripa | Wi-Fi nastavljen, a več kot 30 s brez povezave |
| zelena (zatemnjena) | sveti | pripravljeno (izklopljivo na plošči) |
| rdeča | sveti | nastavitvena plošča je vklopljena |
| rumena | sveti | varni način |
| bela | 1 utrip | prejet Wake-on-LAN ali pritisnjen BOOT |
| turkizna | hitro utripa | poskus zbujanja poteka |
| zelena | 3 utripi | konzola se je odzvala |
| rdeča | 3 utripi | konzola se ni odzvala |
| oranžna | 1 utrip | zahteva prezrta (poskus poteka ali 30 s premora) |

## Nastavitvena plošča

- Plošča in njeno omrežje se izklopita po shranjevanju, po **Zapri brez sprememb** ali po 10 minutah nedejavnosti. **Podaljšaj** podaljša čas.
- **Popolnoma izklopi LED**: Brez barv, utripanja in bliskov. Le med držanjem gumba BOOT LED še vedno kaže čas držanja.
- **Ime naprave (neobvezno)**: Npr. Dnevna soba. Uporablja se v naslovu plošče (`dnevna-soba.local`), v Home Assistant in v naslovu strani – koristno pri več napravah.
- **Dostop iz domačega omrežja**: Privzeto vklopljeno. Stran ostane stalno dostopna v domačem omrežju na `<IP naprave ESP32>` in `ps5-wake-xxxx.local` – prijava z geslom plošče (uporabniško ime admin, če ga niste spremenili). Omrežje Wi-Fi plošče se ne zažene; za odprtje držite BOOT 5 s.
- **Šifrirana povezava (HTTPS)**: Priporočeno. ESP32 ustvari lastno potrdilo, zato brskalnik ob prvem obisku opozori, da povezava ni zasebna – primerjajte prstni odtis spodaj in nadaljujte. Brez HTTPS se geslo in nastavitve prenašajo po omrežju nešifrirano. Potrdilo se pred iztekom samodejno obnovi (ESP32 dobi čas iz interneta); **Novo potrdilo** takoj ustvari novo.
- **Zbudi PS5 zdaj** preizkusi zbujanje neposredno s plošče.

## Odpravljanje težav

- **LED utripa oranžno:** napačno geslo Wi-Fi, omrežje samo 5 GHz ali šibak signal. Odprite ploščo, preverite Wi-Fi in ESP32 premaknite bližje usmerjevalniku. ESP32 se sam znova zažene, če je 2 minuti brez povezave.
- **Po pošiljanju Wake-on-LAN se nič ne zgodi (ni belega utripa):** paket ne pride do ESP32. Uporabite MAC naprave ESP32, naslov za oddajanje in vrata 9; preverite, da ESP32 ni v omrežju za goste.
- **Rdeči utripi po poskusu:** preverite naslova Bluetooth (po ponovnem seznanjanju krmilnika ju znova preberite) in ESP32 premaknite bližje konzoli.
- **Bralnik ne najde krmilnika:** uporabite podatkovni kabel (ne samo polnilnega) in krmilnik priključite neposredno na računalnik.
- **Brskalnik opozarja na potrdilo:** to je pričakovano – plošča je šifrirana z lastnim potrdilom naprave ESP32. Nadaljujte na stran; plošča prikazuje prstni odtis potrdila.

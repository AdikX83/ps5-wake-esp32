# PS5 Wake – korisnički priručnik

PS5 Wake je mala pločica ESP32-C6 koja uključuje vaš PlayStation 5 kad primi Wake-on-LAN paket ili kad pritisnete njezin gumb. Preko Bluetootha se predstavlja kao vaš upareni DualSense upravljač – baš kao pritisak tipke PS. Radi iz načina mirovanja i kad je konzola potpuno isključena.

## Što vam treba

- Pločica ESP32-C6 s PS5 Wake (testirano: ESP32-C6-DevKitM-1), napajana iz bilo kojeg USB punjača.
- DualSense (ili DualSense Edge) upravljač već uparen s vašim PS5.
- Računalo i **podatkovni** USB kabel – samo jednom, za instalaciju firmvera i očitavanje adresa upravljača.
- Wi-Fi mreža od 2,4 GHz.

## 1. Instalacija firmvera

Otvorite web-instalacijski program (poveznica u README-u projekta) u Chromeu ili Edgeu, spojite pločicu USB kabelom i kliknite **Install**. Novi uređaj svijetli **ljubičasto**. Sam pokreće mrežu ploče za postavke.

## 2. Otvaranje ploče za postavke

1. Držite gumb **BOOT** 5–20 sekundi i pustite ga. LED zasvijetli **crveno**. Kod novog uređaja ploča je već uključena (ljubičasta LED) – preskočite ovaj korak.
2. Na računalu na koje ćete spojiti upravljač spojite se na Wi-Fi mrežu `PS5-WAKE-xxxx`, lozinka `ps5wol-setup`.
3. U pregledniku otvorite **`https://192.168.4.1`** (ili `https://ps5-wake-xxxx.local`). Preglednik upozorava da veza nije privatna jer ESP32 koristi vlastiti certifikat – odaberite **Napredno → Nastavi** (nazivi ovise o pregledniku).
4. Na vrhu stranice odaberite jezik.

## 3. Očitavanje adresa upravljača

1. Spojite DualSense na računalo USB kabelom.
2. U odjeljku **Bluetooth** ploče preuzmite čitač adresa:
   - **Windows:** `ps5-wake-reader.cmd` – pokrenite ga dvostrukim klikom. Ako Windows upozori na nepoznatog izdavača, odaberite **Više informacija → Ipak pokreni**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Čitač prikazuje Bluetooth adrese konzole i upravljača te u međuspremnik kopira **kôd za postavljanje** (`PS5WAKE:…`). Zalijepite ga u polje **Kod za postavljanje iz čitača adresa** – obje adrese se popunjavaju.

Adrese se mijenjaju samo kad ponovno uparite upravljač s konzolom.

## 4. Dovršetak postavljanja

1. Odaberite Wi-Fi mrežu gumbom **Traži**, unesite lozinku i odaberite zemlju.
2. Unesite **Nova lozinka mreže ploče** (najmanje 8 znakova). Obavezna je jer je zadana lozinka javno poznata.
3. Kliknite **Spremi i ponovno pokreni**. Ploča se zatvara, LED tijekom spajanja treperi **plavo**, a kad je sve spremno svijetli **zeleno**.

## 5. Postavljanje Wake-on-LAN

- U aplikaciji za Wake-on-LAN (ili na WoL stranici usmjerivača) koristite **MAC adresa ovog ESP32** s ploče, UDP priključak **9** i **broadcast adresu** mreže (npr. `192.168.1.255`).
- U usmjerivaču rezervirajte stalnu IP adresu za ESP32.
- Ako aplikacija ili usmjerivač već koriste **mrežni MAC samog PS5**, možete ga unijeti u polje **Reagiraj i na MAC same konzole PS5** – ESP32 će reagirati i na njega. U mesh mrežama (npr. ASUS AiMesh) takvi paketi možda neće stići do ESP32; tada šaljite WoL na MAC ESP32.

## Svakodnevna upotreba

- **Stanje konzole:** točka na ikoni napajanja na vrhu ploče zelena je kad je PS5 uključen, narančasta u načinu mirovanja i crvena kad je isključen ili nedostupan (siva: još nije pronađen). Način mirovanja prepoznaje se samo ako konzola ostaje povezana s internetom.
- **Buđenje PS5:** pošaljite Wake-on-LAN paket ili kratko pritisnite **BOOT**. LED jednom bljesne bijelo, tijekom pokušaja treperi tirkizno, a zatim bljesne **3× zeleno** (konzola je odgovorila) ili **3× crveno** (nema odgovora).
- Između pokušaja prolazi najmanje 30 sekundi; zanemareni zahtjev daje jedan narančasti bljesak.

## Gumb BOOT

| Vrijeme držanja | LED tijekom držanja | Rezultat |
|---|---|---|
| manje od 1 s | – | buđenje PS5 |
| 1–5 s | bijela | ništa (poništavanje) |
| 5–20 s, zatim pustiti | crvena (od 15 s treperi) | otvaranje ploče za postavke |
| 20 s | ljubičasta, brzo treperi | brisanje svih postavki i ponovno pokretanje |

Držanje BOOT pri spajanju napajanja pokreće sigurni način rada bez Wi-Fi-ja i Bluetootha (žuta LED).

## Boje LED-a

| Boja | Ponašanje | Značenje |
|---|---|---|
| ljubičasta | svijetli | nije postavljeno – ploča za postavke je uključena |
| plava | sporo treperi | spajanje na Wi-Fi |
| narančasta | sporo treperi | Wi-Fi postavljen, ali bez veze dulje od 30 s |
| zelena (prigušena) | svijetli | spremno (može se isključiti na ploči) |
| crvena | svijetli | ploča za postavke je uključena |
| žuta | svijetli | sigurni način rada |
| bijela | 1 bljesak | primljen Wake-on-LAN ili pritisnut BOOT |
| tirkizna | brzo treperi | pokušaj buđenja u tijeku |
| zelena | 3 bljeska | konzola je odgovorila |
| crvena | 3 bljeska | konzola nije odgovorila |
| narančasta | 1 bljesak | zahtjev zanemaren (pokušaj u tijeku ili 30 s pauze) |

## Ploča za postavke

- Ploča i njezina mreža isključuju se nakon spremanja, nakon **Zatvori bez promjena** ili nakon 10 minuta bez aktivnosti. **Produlji** daje više vremena.
- **Potpuno isključi LED**: Bez boja, treperenja i bljeskova. Samo dok držite BOOT, LED i dalje pokazuje vrijeme držanja.
- **Naziv uređaja (neobavezno)**: Npr. Dnevni boravak. Koristi se u adresi ploče (`dnevni-boravak.local`), u Home Assistantu i u naslovu stranice – korisno uz više uređaja.
- **Pristup iz kućne mreže**: Zadano uključeno. Stranica ostaje stalno dostupna u kućnoj mreži na `<IP ESP32>` i `ps5-wake-xxxx.local` – prijava lozinkom ploče (korisničko ime admin, osim ako ste ga promijenili). Wi-Fi mreža ploče se ne pokreće; za otvaranje držite BOOT 5 s.
- **Šifrirana veza (HTTPS)**: Preporučeno. ESP32 izrađuje vlastiti certifikat pa će preglednik pri prvom posjetu upozoriti da veza nije privatna – usporedite otisak u nastavku i nastavite. Bez HTTPS-a lozinka i postavke putuju mrežom nešifrirano. Certifikat se automatski obnavlja prije isteka (ESP32 dohvaća vrijeme s interneta); **Novi certifikat** odmah izrađuje novi.
- **Probudi PS5 sada** testira buđenje izravno s ploče.

## Rješavanje problema

- **LED treperi narančasto:** pogrešna Wi-Fi lozinka, mreža samo od 5 GHz ili slab signal. Otvorite ploču, provjerite Wi-Fi i približite ESP32 usmjerivaču. ESP32 se sam ponovno pokreće ako je 2 minute bez veze.
- **Nakon slanja Wake-on-LAN ništa se ne događa (nema bijelog bljeska):** paket ne stiže do ESP32. Koristite MAC ESP32, broadcast adresu i priključak 9; provjerite da ESP32 nije u mreži za goste.
- **Crveni bljeskovi nakon pokušaja:** provjerite Bluetooth adrese (ponovno ih očitajte nakon ponovnog uparivanja upravljača) i približite ESP32 konzoli.
- **Čitač ne pronalazi upravljač:** koristite podatkovni kabel (ne samo za punjenje) i spojite upravljač izravno na računalo.
- **Preglednik upozorava na certifikat:** to je očekivano – ploča je šifrirana vlastitim certifikatom ESP32. Nastavite na stranicu; ploča prikazuje otisak certifikata.

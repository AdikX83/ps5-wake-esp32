# PS5 Wake – gwida għall-utent

PS5 Wake huwa bord żgħir ESP32-C6 li jixgħel il-PlayStation 5 tiegħek meta jirċievi pakkett Wake-on-LAN jew meta tagħfas il-buttuna tiegħu. Permezz tal-Bluetooth jippreżenta ruħu bħala l-kontroller DualSense imqabbad tiegħek – eżatt bħal meta tagħfas il-buttuna PS. Jaħdem mill-modalità ta’ mistrieħ u minn konsol mitfi għalkollox.

## X’għandek bżonn

- Bord ESP32-C6 b’PS5 Wake (ittestjat: ESP32-C6-DevKitM-1), imħaddem minn kwalunkwe ċarġer USB.
- Kontroller DualSense (jew DualSense Edge) diġà mqabbad mal-PS5 tiegħek.
- Kompjuter u kejbil USB **tad-data** – darba biss, biex tinstalla l-firmware u taqra l-indirizzi tal-kontroller.
- Netwerk Wi-Fi ta’ 2.4 GHz.

## 1. Installa l-firmware

Iftaħ l-installatur tal-web (link fir-README tal-proġett) f’Chrome jew Edge, qabbad il-bord b’kejbil USB u kklikkja **Install or update**. Apparat ġdid jixgħel **vjola**. Jibda n-netwerk tal-pannell tal-konfigurazzjoni waħdu.

## 2. Iftaħ il-pannell tal-konfigurazzjoni

1. Żomm magħfusa l-buttuna **BOOT** għal 5–20 sekonda u erħiha. L-LED isir **aħmar**. F’apparat ġdid il-pannell ikun diġà mixgħul (LED vjola) – aqbeż dan il-pass.
2. Fuq il-kompjuter li se tqabbad il-kontroller miegħu, qabbad man-netwerk Wi-Fi `PS5-WAKE-xxxx`, password `ps5wol-setup`.
3. Fil-browser iftaħ **`https://192.168.4.1`** (jew `https://ps5-wake-xxxx.local`). Il-browser iwissik li l-konnessjoni mhix privata, għax l-ESP32 juża ċ-ċertifikat tiegħu – agħżel **Avvanzat → Kompli** (l-ismijiet jiddependu mill-browser).
4. Agħżel il-lingwa fin-naħa ta’ fuq tal-paġna.

## 3. Aqra l-indirizzi tal-kontroller

1. Qabbad id-DualSense mal-kompjuter bil-kejbil USB.
2. Fit-taqsima **Bluetooth** tal-pannell, niżżel il-qarrej tal-indirizzi:
   - **Windows:** `ps5-wake-reader.cmd` – iftħu b’double-click. Jekk Windows iwissik dwar pubblikatur mhux magħruf, agħżel **Aktar informazzjoni → Ħaddem xorta waħda**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Il-qarrej juri l-indirizzi Bluetooth tal-konsol u tal-kontroller u jikkopja **kodiċi tal-konfigurazzjoni** (`PS5WAKE:…`) fil-clipboard. Waħħlu fil-qasam **Kodiċi tal-konfigurazzjoni mill-qarrej** – iż-żewġ indirizzi jimtlew.

L-indirizzi jinbidlu biss jekk terġa’ tqabbad il-kontroller mal-konsol.

## 4. Lesti l-konfigurazzjoni

1. Agħżel in-netwerk Wi-Fi bi **Fittex**, daħħal il-password tiegħu u agħżel il-pajjiż.
2. Daħħal **Password ġdida għan-netwerk tal-pannell** (mill-inqas 8 karattri). Hija meħtieġa għax il-password awtomatika hija pubblika.
3. Ikklikkja **Issejvja u erġa’ ibda**. Il-pannell jagħlaq, l-LED iteptep **blu** waqt il-konnessjoni u jsir **aħdar** meta kollox ikun lest.

## 5. Ikkonfigura Wake-on-LAN

- Fl-app tal-Wake-on-LAN (jew fil-paġna WoL tar-router) uża **MAC ta’ dan l-ESP32** mill-pannell, il-port UDP **9** u l-**indirizz tax-xandir** tan-netwerk (eż. `192.168.1.255`).
- Irriżerva indirizz IP fiss għall-ESP32 fir-router.
- Jekk l-app jew ir-router diġà jużaw il-**MAC tan-netwerk tal-PS5 innifisha**, tista’ ddaħħalha fil-qasam **Irreaġixxi wkoll għall-MAC tal-PS5 innifsu** – l-ESP32 jirreaġixxi wkoll għaliha. F’netwerks mesh (eż. ASUS AiMesh) dawn il-pakketti jistgħu ma jaslux għand l-ESP32; f’dak il-każ ibgħat il-WoL lill-MAC tal-ESP32.

## Użu ta’ kuljum

- **Stat tal-konsol:** it-tikka fuq l-ikona tal-qawwa fil-parti ta’ fuq tal-pannell hija ħadra meta l-PS5 tkun mixgħula, oranġjo fil-modalità ta’ mistrieħ u ħamra meta tkun mitfija jew mhux disponibbli (griża: għadha ma nstabitx). Il-modalità ta’ mistrieħ tinstab biss jekk il-konsol tibqa’ konnessa mal-internet.
- **Qajjem il-PS5:** ibgħat pakkett Wake-on-LAN jew agħfas ftit **BOOT**. L-LED jagħti teptipa bajda, iteptep ċjan waqt it-tentattiv u mbagħad jagħti **3 teptipiet ħodor** (il-konsol wieġeb) jew **3 teptipiet ħomor** (l-ebda tweġiba).
- Bejn tentattiv u ieħor jgħaddu mill-inqas 30 sekonda; talba injorata tagħti teptipa oranġjo waħda.

## Il-buttuna BOOT

| Kemm iżżommha | LED waqt li żżommha | Riżultat |
|---|---|---|
| inqas minn 1 s | – | tqajjem il-PS5 |
| 1–5 s | abjad | xejn (tħassir) |
| 5–20 s, imbagħad erħi | aħmar (iteptep minn 15 s) | tiftaħ il-pannell tal-konfigurazzjoni |
| 20 s | vjola, iteptep malajr | tħassar is-settings kollha u terġa’ tibda |

Jekk iżżomm BOOT magħfusa waqt li tqabbad il-kurrent, tibda modalità sigura mingħajr Wi-Fi u Bluetooth (LED isfar).

## Kuluri tal-LED

| Kulur | Imġiba | Tifsira |
|---|---|---|
| vjola | fiss | mhux ikkonfigurat – il-pannell tal-konfigurazzjoni mixgħul |
| blu | iteptep bil-mod | qed jikkonnettja mal-Wi-Fi |
| oranġjo | iteptep bil-mod | Wi-Fi kkonfigurat iżda mhux konness għal aktar minn 30 s |
| aħdar (baxx) | fiss | lest (jista’ jintefa mill-pannell) |
| aħmar | fiss | il-pannell tal-konfigurazzjoni mixgħul |
| isfar | fiss | modalità sigura |
| abjad | teptipa waħda | Wake-on-LAN riċevut jew BOOT magħfus |
| ċjan | iteptep malajr | tentattiv ta’ tqajjim għaddej |
| aħdar | tliet teptipiet | il-konsol wieġeb |
| aħmar | tliet teptipiet | il-konsol ma wieġibx |
| oranġjo | teptipa waħda | talba injorata (tentattiv għaddej jew pawża ta’ 30 s) |

## Il-pannell tal-konfigurazzjoni

- Il-pannell u n-netwerk tiegħu jintfew wara li tissejvja, wara **Agħlaq mingħajr bidliet** jew wara 10 minuti mingħajr attività. **Estendi** jagħtik aktar ħin.
- **Itfi l-LED għalkollox**: L-ebda kulur, teptip jew flash. Biss waqt li żżomm BOOT magħfusa l-LED għadu juri kemm ilek iżżommha.
- **Isem tal-apparat (mhux obbligatorju)**: Eż. Salott. Jintuża fl-indirizz tal-pannell (`salott.local`), f’Home Assistant u fit-titlu tal-paġna – utli b’aktar minn apparat wieħed.
- **Aċċess min-netwerk tad-dar**: Mixgħul awtomatikament. Il-paġna tibqa’ disponibbli fin-netwerk tad-dar fuq `<IP tal-ESP32>` u `ps5-wake-xxxx.local` – idħol bil-password tal-pannell (isem tal-utent admin sakemm ma biddiltux). In-netwerk Wi-Fi tal-pannell ma jinxtegħelx; żomm BOOT għal 5 s biex tiftħu.
- **Konnessjoni kriptata (HTTPS)**: Rakkomandat. L-ESP32 joħloq iċ-ċertifikat tiegħu, għalhekk fl-ewwel żjara l-browser iwissik li l-konnessjoni mhix privata – qabbel il-marka ta’ hawn taħt u kompli. Mingħajr HTTPS il-password u s-settings jgħaddu fin-netwerk mingħajr kriptaġġ. Iċ-ċertifikat jiġġedded awtomatikament qabel ma jiskadi (l-ESP32 jieħu l-ħin mill-internet); **Ċertifikat ġdid** joħloq wieħed ġdid minnufih.
- **Qajjem il-PS5 issa** jittestja t-tqajjim direttament mill-pannell.

## Soluzzjoni tal-problemi

- **L-LED iteptep oranġjo:** password tal-Wi-Fi ħażina, netwerk ta’ 5 GHz biss jew sinjal dgħajjef. Iftaħ il-pannell, iċċekkja l-Wi-Fi u ressaq l-ESP32 eqreb lejn ir-router. L-ESP32 jerġa’ jibda waħdu jekk jibqa’ offline għal 2 minuti.
- **Ma jiġri xejn wara li tibgħat Wake-on-LAN (l-ebda teptipa bajda):** il-pakkett ma jasalx għand l-ESP32. Uża l-MAC tal-ESP32, l-indirizz tax-xandir u l-port 9; iċċekkja li l-ESP32 mhux fuq netwerk tal-mistednin.
- **Teptipiet ħomor wara tentattiv:** iċċekkja l-indirizzi Bluetooth (erġa’ aqrahom jekk qabbadt mill-ġdid il-kontroller) u ressaq l-ESP32 eqreb lejn il-konsol.
- **Il-qarrej ma jsibx il-kontroller:** uża kejbil tad-data (mhux tal-iċċarġjar biss) u qabbad il-kontroller direttament mal-kompjuter.
- **Il-browser iwissi dwar iċ-ċertifikat:** dan huwa mistenni – il-pannell huwa kriptat biċ-ċertifikat tal-ESP32 stess. Kompli għall-paġna; il-pannell juri l-marka taċ-ċertifikat.

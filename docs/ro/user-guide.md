# PS5 Wake – ghid de utilizare

PS5 Wake este o plăcuță mică ESP32-C6 care pornește PlayStation 5 când primește un pachet Wake-on-LAN sau când apăsați butonul ei. Prin Bluetooth se prezintă drept controlerul DualSense asociat – exact ca o apăsare a butonului PS. Funcționează din modul de repaus și cu consola complet oprită.

## Ce vă trebuie

- O plăcuță ESP32-C6 cu PS5 Wake (testată: ESP32-C6-DevKitM-1), alimentată de la orice încărcător USB.
- Un controler DualSense (sau DualSense Edge) deja asociat cu PS5.
- Un calculator și un cablu USB **de date** – o singură dată, pentru instalarea firmware-ului și citirea adreselor controlerului.
- O rețea Wi-Fi de 2,4 GHz.

## 1. Instalarea firmware-ului

Deschideți instalatorul web (linkul din README-ul proiectului) în Chrome sau Edge, conectați plăcuța cu un cablu USB și faceți clic pe **Install or update**. Un dispozitiv nou luminează **mov**. Pornește singur rețeaua panoului de configurare.

## 2. Deschiderea panoului de configurare

1. Țineți apăsat butonul **BOOT** 5–20 de secunde, apoi eliberați-l. LED-ul devine **roșu**. La un dispozitiv nou panoul este deja pornit (LED mov) – săriți peste acest pas.
2. Pe calculatorul la care veți conecta controlerul, conectați-vă la rețeaua Wi-Fi `PS5-WAKE-xxxx`, parola `ps5wol-setup`.
3. În browser deschideți **`https://192.168.4.1`** (sau `https://ps5-wake-xxxx.local`). Browserul avertizează că conexiunea nu este privată, deoarece ESP32 folosește propriul certificat – alegeți **Avansat → Continuați** (denumirile depind de browser).
4. Alegeți limba în partea de sus a paginii.

## 3. Citirea adreselor controlerului

1. Conectați DualSense la calculator cu cablul USB.
2. În secțiunea **Bluetooth** a panoului, descărcați cititorul de adrese:
   - **Windows:** `ps5-wake-reader.cmd` – porniți-l cu dublu clic. Dacă Windows avertizează despre un editor necunoscut, alegeți **Mai multe informații → Executare oricum**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Cititorul afișează adresele Bluetooth ale consolei și controlerului și copiază în clipboard un **cod de configurare** (`PS5WAKE:…`). Lipiți-l în câmpul **Cod de configurare din cititorul de adrese** – ambele adrese se completează.

Adresele se schimbă doar dacă asociați din nou controlerul cu consola.

## 4. Finalizarea configurării

1. Alegeți rețeaua Wi-Fi cu **Caută**, introduceți parola și alegeți țara.
2. Introduceți **Parolă nouă pentru rețeaua panoului** (cel puțin 8 caractere). Este obligatorie, deoarece parola implicită este publică.
3. Faceți clic pe **Salvează și repornește**. Panoul se închide, LED-ul clipește **albastru** în timpul conectării și devine **verde** când totul este gata.

## 5. Configurarea Wake-on-LAN

- În aplicația Wake-on-LAN (sau pe pagina WoL a routerului) folosiți **Adresa MAC a acestui ESP32** afișată în panou, portul UDP **9** și **adresa de difuzare** a rețelei (de ex. `192.168.1.255`).
- Rezervați în router o adresă IP fixă pentru ESP32.
- Dacă aplicația sau routerul folosesc deja **adresa MAC de rețea a PS5**, o puteți introduce în câmpul **Reacționează și la adresa MAC a PS5** – ESP32 va reacționa și la ea. În rețelele mesh (de ex. ASUS AiMesh) aceste pachete pot să nu ajungă la ESP32; atunci trimiteți WoL către adresa MAC a ESP32.

## Utilizare zilnică

- **Starea consolei:** punctul de pe pictograma de pornire din partea de sus a panoului este verde când PS5 este pornită, portocaliu în modul de repaus și roșu când este oprită sau inaccesibilă (gri: încă negăsită). Modul de repaus este detectat doar dacă consola rămâne conectată la internet.
- **Trezirea PS5:** trimiteți un pachet Wake-on-LAN sau apăsați scurt **BOOT**. LED-ul clipește o dată alb, clipește turcoaz în timpul încercării, apoi clipește **3× verde** (consola a răspuns) sau **3× roșu** (fără răspuns).
- Între încercări trec cel puțin 30 de secunde; o cerere ignorată produce o clipire portocalie.

## Butonul BOOT

| Durata apăsării | LED în timpul apăsării | Rezultat |
|---|---|---|
| sub 1 s | – | trezirea PS5 |
| 1–5 s | alb | nimic (anulare) |
| 5–20 s, apoi eliberare | roșu (clipește de la 15 s) | deschiderea panoului de configurare |
| 20 s | mov, clipește rapid | ștergerea tuturor setărilor și repornire |

Ținerea apăsată a BOOT la conectarea alimentării pornește un mod sigur fără Wi-Fi și Bluetooth (LED galben).

## Culorile LED-ului

| Culoare | Comportament | Semnificație |
|---|---|---|
| mov | continuu | neconfigurat – panoul de configurare este pornit |
| albastru | clipește lent | conectare la Wi-Fi |
| portocaliu | clipește lent | Wi-Fi configurat, dar neconectat de peste 30 s |
| verde (estompat) | continuu | pregătit (se poate dezactiva din panou) |
| roșu | continuu | panoul de configurare este pornit |
| galben | continuu | mod sigur |
| alb | o clipire | Wake-on-LAN primit sau BOOT apăsat |
| turcoaz | clipește rapid | încercare de trezire în curs |
| verde | trei clipiri | consola a răspuns |
| roșu | trei clipiri | consola nu a răspuns |
| portocaliu | o clipire | cerere ignorată (încercare în curs sau pauză de 30 s) |

## Panoul de configurare

- Panoul și rețeaua lui se opresc după salvare, după **Închide fără modificări** sau după 10 minute fără activitate. **Prelungește** oferă mai mult timp.
- **Oprește complet LED-ul**: Fără culori, clipiri sau flash-uri. Doar cât timp țineți apăsat BOOT, LED-ul arată în continuare durata apăsării.
- **Numele dispozitivului (opțional)**: De ex. Sufragerie. Folosit în adresa panoului (`sufragerie.local`), în Home Assistant și în titlul paginii – util cu mai multe dispozitive.
- **Acces din rețeaua de acasă**: Activat implicit. Pagina rămâne disponibilă permanent în rețeaua de acasă la `<IP ESP32>` și `ps5-wake-xxxx.local` – autentificare cu parola panoului (nume de utilizator admin, dacă nu l-ați schimbat). Rețeaua Wi-Fi a panoului nu este pornită; țineți BOOT 5 s pentru a o deschide.
- **Conexiune criptată (HTTPS)**: Recomandat. ESP32 își creează propriul certificat, așa că la prima vizită browserul avertizează că conexiunea nu este privată – comparați amprenta de mai jos și continuați. Fără HTTPS, parola și setările circulă necriptat în rețea. Certificatul se reînnoiește automat înainte de expirare (ESP32 preia ora de pe internet); **Certificat nou** creează imediat unul nou.
- **Trezește PS5 acum** testează trezirea direct din panou.

## Depanare

- **LED-ul clipește portocaliu:** parolă Wi-Fi greșită, rețea doar de 5 GHz sau semnal slab. Deschideți panoul, verificați Wi-Fi și apropiați ESP32 de router. ESP32 repornește singur dacă rămâne 2 minute fără conexiune.
- **Nu se întâmplă nimic după trimiterea Wake-on-LAN (nicio clipire albă):** pachetul nu ajunge la ESP32. Folosiți adresa MAC a ESP32, adresa de difuzare și portul 9; verificați că ESP32 nu este într-o rețea pentru oaspeți.
- **Clipiri roșii după o încercare:** verificați adresele Bluetooth (citiți-le din nou după reasocierea controlerului) și apropiați ESP32 de consolă.
- **Cititorul nu găsește controlerul:** folosiți un cablu de date (nu doar de încărcare) și conectați controlerul direct la calculator.
- **Browserul avertizează despre certificat:** este normal – panoul este criptat cu certificatul propriu al ESP32. Continuați spre pagină; panoul afișează amprenta certificatului.

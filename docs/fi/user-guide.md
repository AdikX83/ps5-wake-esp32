# PS5 Wake – käyttöopas

PS5 Wake on pieni ESP32-C6-piirilevy, joka käynnistää PlayStation 5:n, kun se vastaanottaa Wake-on-LAN-paketin tai kun painat sen painiketta. Bluetoothin kautta se esiintyy pariliitettynä DualSense-ohjaimenasi – aivan kuin painaisit PS-painiketta. Toimii lepotilasta ja täysin sammutetusta konsolista.

## Mitä tarvitset

- ESP32-C6-piirilevy, jossa on PS5 Wake (testattu: ESP32-C6-DevKitM-1), virta mistä tahansa USB-laturista.
- DualSense- (tai DualSense Edge -)ohjain, joka on jo pariliitetty PS5:een.
- Tietokone ja **datansiirtoon sopiva** USB-kaapeli – vain kerran, laiteohjelmiston asentamiseen ja ohjaimen osoitteiden lukemiseen.
- 2,4 GHz:n Wi-Fi-verkko.

## 1. Asenna laiteohjelmisto

Avaa verkkoasennusohjelma (linkki projektin README-tiedostossa) Chromessa tai Edgessä, liitä levy USB-kaapelilla ja napsauta **Install or update**. Uusi laite palaa **violettina**. Se käynnistää asetuspaneelin verkon itsestään.

## 2. Avaa asetuspaneeli

1. Pidä **BOOT**-painiketta painettuna 5–20 sekuntia ja vapauta se. LED muuttuu **punaiseksi**. Uudessa laitteessa paneeli on jo päällä (violetti LED) – ohita tämä vaihe.
2. Yhdistä tietokone, johon liität ohjaimen, Wi-Fi-verkkoon `PS5-WAKE-xxxx`, salasana `ps5wol-setup`.
3. Avaa selaimessa **`https://192.168.4.1`** (tai `https://ps5-wake-xxxx.local`). Selain varoittaa, ettei yhteys ole yksityinen, koska ESP32 käyttää omaa varmennettaan – valitse **Lisäasetukset → Jatka** (nimet vaihtelevat selaimittain).
4. Valitse kieli sivun yläosasta.

## 3. Lue ohjaimen osoitteet

1. Liitä DualSense tietokoneeseen USB-kaapelilla.
2. Lataa osoitteenlukija paneelin osiosta **Bluetooth**:
   - **Windows:** `ps5-wake-reader.cmd` – käynnistä se kaksoisnapsautuksella. Jos Windows varoittaa tuntemattomasta julkaisijasta, valitse **Lisätietoja → Suorita silti**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Lukija näyttää konsolin ja ohjaimen Bluetooth-osoitteet ja kopioi leikepöydälle **asetuskoodin** (`PS5WAKE:…`). Liitä se kenttään **Osoitteenlukijan asetuskoodi** – molemmat osoitteet täyttyvät.

Osoitteet muuttuvat vain, jos pariliität ohjaimen konsoliin uudelleen.

## 4. Viimeistele asetukset

1. Valitse Wi-Fi-verkko painikkeella **Etsi**, anna sen salasana ja valitse maa.
2. Anna **Paneelin verkon uusi salasana** (vähintään 8 merkkiä). Se on pakollinen, koska oletussalasana on julkisesti tiedossa.
3. Napsauta **Tallenna ja käynnistä uudelleen**. Paneeli sulkeutuu, LED vilkkuu **sinisenä** yhdistämisen aikana ja palaa **vihreänä**, kun kaikki on valmista.

## 5. Määritä Wake-on-LAN

- Käytä Wake-on-LAN-sovelluksessa (tai reitittimen WoL-sivulla) paneelissa näkyvää kohtaa **Tämän ESP32:n MAC-osoite**, UDP-porttia **9** ja verkon **yleislähetysosoitetta** (esim. `192.168.1.255`).
- Varaa reitittimestä ESP32:lle kiinteä IP-osoite.
- Jos sovellus tai reititin käyttää jo **PS5:n omaa verkko-MAC-osoitetta**, voit syöttää sen kenttään **Reagoi myös PS5:n omaan MAC-osoitteeseen** – ESP32 reagoi silloin myös siihen. Mesh-verkoissa (esim. ASUS AiMesh) tällaiset paketit eivät välttämättä tavoita ESP32:ta; lähetä WoL silloin ESP32:n MAC-osoitteeseen.

## Päivittäinen käyttö

- **Konsolin tila:** paneelin yläosan virtakuvakkeen piste on vihreä, kun PS5 on päällä, oranssi lepotilassa ja punainen, kun se on pois päältä tai ei tavoitettavissa (harmaa: ei vielä löydetty). Lepotila tunnistetaan vain, jos konsoli pysyy siinä yhteydessä internetiin.
- **PS5:n herättäminen:** lähetä Wake-on-LAN-paketti tai paina lyhyesti **BOOT**. LED välähtää kerran valkoisena, vilkkuu turkoosina yrityksen aikana ja välähtää sitten **3× vihreänä** (konsoli vastasi) tai **3× punaisena** (ei vastausta).
- Yritysten välillä on vähintään 30 sekuntia; ohitettu pyyntö näkyy yhtenä oranssina välähdyksenä.

## BOOT-painike

| Painalluksen kesto | LED painettaessa | Tulos |
|---|---|---|
| alle 1 s | – | PS5:n herättäminen |
| 1–5 s | valkoinen | ei mitään (peruutus) |
| 5–20 s, sitten vapautus | punainen (vilkkuu 15 s:sta) | asetuspaneelin avaaminen |
| 20 s | violetti, vilkkuu nopeasti | kaikkien asetusten poisto ja uudelleenkäynnistys |

Jos pidät BOOT-painiketta painettuna virtaa kytkettäessä, käynnistyy vikasietotila ilman Wi-Fiä ja Bluetoothia (keltainen LED).

## LEDin värit

| Väri | Toiminta | Merkitys |
|---|---|---|
| violetti | palaa | ei määritetty – asetuspaneeli on päällä |
| sininen | vilkkuu hitaasti | yhdistetään Wi-Fiin |
| oranssi | vilkkuu hitaasti | Wi-Fi määritetty, mutta ei yhteyttä yli 30 s |
| vihreä (himmeä) | palaa | valmis (voi sammuttaa paneelista) |
| punainen | palaa | asetuspaneeli on päällä |
| keltainen | palaa | vikasietotila |
| valkoinen | 1 välähdys | Wake-on-LAN vastaanotettu tai BOOT painettu |
| turkoosi | vilkkuu nopeasti | herätysyritys käynnissä |
| vihreä | 3 välähdystä | konsoli vastasi |
| punainen | 3 välähdystä | konsoli ei vastannut |
| oranssi | 1 välähdys | pyyntö ohitettu (yritys käynnissä tai 30 s tauko) |

## Asetuspaneeli

- Paneeli ja sen verkko sammuvat tallennuksen jälkeen, kohdan **Sulje muuttamatta** jälkeen tai 10 minuutin käyttämättömyyden jälkeen. **Jatka** antaa lisäaikaa.
- **Sammuta LED kokonaan**: Ei värejä, vilkkumista eikä välähdyksiä. Vain BOOT-painiketta painettaessa LED näyttää edelleen painalluksen keston.
- **Laitteen nimi (valinnainen)**: Esim. Olohuone. Käytetään paneelin osoitteessa (`olohuone.local`), Home Assistantissa ja sivun otsikossa – hyödyllinen, jos laitteita on useita.
- **Käyttö kotiverkosta**: Oletuksena päällä. Sivu on jatkuvasti käytettävissä kotiverkossa osoitteissa `<ESP32:n IP>` ja `ps5-wake-xxxx.local` – kirjaudu paneelin salasanalla (käyttäjänimi admin, ellet ole vaihtanut sitä). Paneelin Wi-Fi-verkkoa ei käynnistetä; avaa se pitämällä BOOT-painiketta 5 s.
- **Salattu yhteys (HTTPS)**: Suositeltava. ESP32 luo oman varmenteen, joten ensimmäisellä käyntikerralla selain varoittaa, ettei yhteys ole yksityinen – vertaa alla olevaa sormenjälkeä ja jatka. Ilman HTTPS:ää salasana ja asetukset kulkevat verkossa salaamattomina. Varmenne uusitaan automaattisesti ennen vanhenemista (ESP32 hakee ajan internetistä); **Uusi varmenne** luo uuden heti.
- **Herätä PS5 nyt** testaa herättämisen suoraan paneelista.

## Vianmääritys

- **LED vilkkuu oranssina:** väärä Wi-Fi-salasana, pelkkä 5 GHz:n verkko tai heikko signaali. Avaa paneeli, tarkista Wi-Fi ja siirrä ESP32 lähemmäs reititintä. ESP32 käynnistyy itsestään uudelleen, jos se on 2 minuuttia ilman yhteyttä.
- **Wake-on-LAN-paketin lähettämisen jälkeen ei tapahdu mitään (ei valkoista välähdystä):** paketti ei tavoita ESP32:ta. Käytä ESP32:n MAC-osoitetta, yleislähetysosoitetta ja porttia 9; tarkista, ettei ESP32 ole vierasverkossa.
- **Punaisia välähdyksiä yrityksen jälkeen:** tarkista Bluetooth-osoitteet (lue ne uudelleen, jos pariliitit ohjaimen uudelleen) ja siirrä ESP32 lähemmäs konsolia.
- **Lukija ei löydä ohjainta:** käytä datakaapelia (ei pelkkää latauskaapelia) ja liitä ohjain suoraan tietokoneeseen.
- **Selain varoittaa varmenteesta:** tämä on odotettua – paneeli salataan ESP32:n omalla varmenteella. Jatka sivulle; paneeli näyttää varmenteen sormenjäljen.

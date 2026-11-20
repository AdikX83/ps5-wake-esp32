# PS5 Wake – felhasználói útmutató

A PS5 Wake egy kis ESP32-C6 panel, amely bekapcsolja a PlayStation 5-öt, ha Wake-on-LAN csomagot kap, vagy ha megnyomja a gombját. Bluetooth-on a párosított DualSense kontrollernek adja ki magát – pontosan úgy, mint a PS gomb megnyomása. Nyugalmi módból és teljesen kikapcsolt konzolról is működik.

## Amire szükség van

- Egy ESP32-C6 panel PS5 Wake-kel (tesztelve: ESP32-C6-DevKitM-1), bármilyen USB-töltőről táplálva.
- Egy DualSense (vagy DualSense Edge) kontroller, amely már párosítva van a PS5-tel.
- Egy számítógép és egy **adatátvitelre alkalmas** USB-kábel – csak egyszer, a firmware telepítéséhez és a kontroller címeinek kiolvasásához.
- Egy 2,4 GHz-es Wi-Fi hálózat.

## 1. A firmware telepítése

Nyissa meg a webes telepítőt (hivatkozás a projekt README-jében) Chrome-ban vagy Edge-ben, csatlakoztassa a panelt USB-kábellel, és kattintson az **Install or update** gombra. Az új eszköz **lilán** világít. Magától elindítja a beállítópanel hálózatát.

## 2. A beállítópanel megnyitása

1. Tartsa lenyomva a **BOOT** gombot 5–20 másodpercig, majd engedje el. A LED **pirosan** világít. Új eszközön a panel már be van kapcsolva (lila LED) – hagyja ki ezt a lépést.
2. Azon a számítógépen, amelyhez a kontrollert csatlakoztatja, csatlakozzon a(z) `PS5-WAKE-xxxx` Wi-Fi hálózathoz, jelszó: `ps5wol-setup`.
3. A böngészőben nyissa meg a(z) **`https://192.168.4.1`** (vagy `https://ps5-wake-xxxx.local`) címet. A böngésző figyelmeztet, hogy a kapcsolat nem privát, mert az ESP32 saját tanúsítványt használ – válassza a **Speciális → Tovább** lehetőséget (az elnevezés böngészőnként eltér).
4. Az oldal tetején válassza ki a nyelvet.

## 3. A kontroller címeinek kiolvasása

1. Csatlakoztassa a DualSense-t USB-kábellel a számítógéphez.
2. A panel **Bluetooth** részében töltse le a címolvasót:
   - **Windows:** `ps5-wake-reader.cmd` – indítsa dupla kattintással. Ha a Windows ismeretlen közzétevőre figyelmeztet, válassza a **További információ → Futtatás mindenképp** lehetőséget.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Az olvasó megjeleníti a konzol és a kontroller Bluetooth-címét, és a vágólapra másol egy **beállítókódot** (`PS5WAKE:…`). Illessze be a(z) **Beállítókód a címolvasóból** mezőbe – mindkét cím kitöltődik.

A címek csak akkor változnak, ha a kontrollert újra párosítja a konzollal.

## 4. A beállítás befejezése

1. Válassza ki a Wi-Fi hálózatot a **Keresés** gombbal, adja meg a jelszavát, és válassza ki az országot.
2. Adja meg: **A panel hálózatának új jelszava** (legalább 8 karakter). Kötelező, mert az alapértelmezett jelszó nyilvános.
3. Kattintson a(z) **Mentés és újraindítás** gombra. A panel bezárul, a LED csatlakozás közben **kéken** villog, majd **zölden** világít, amikor minden kész.

## 5. A Wake-on-LAN beállítása

- A Wake-on-LAN alkalmazásban (vagy a router WoL oldalán) használja a panelen látható **Ennek az ESP32-nek a MAC-címe** címet, a **9**-es UDP-portot és a hálózat **broadcast címét** (pl. `192.168.1.255`).
- Foglaljon le fix IP-címet az ESP32 számára a routerben.
- Ha az alkalmazás vagy a router már a **PS5 saját hálózati MAC-címét** használja, megadhatja a(z) **Reagáljon a PS5 saját MAC-címére is** mezőben – az ESP32 arra is reagál. Mesh-hálózatokban (pl. ASUS AiMesh) az ilyen csomagok nem biztos, hogy eljutnak az ESP32-höz; ilyenkor az ESP32 MAC-címére küldje a WoL-t.

## Mindennapi használat

- **Konzol állapota:** a panel tetején a bekapcsoló ikonon lévő pont zöld, ha a PS5 be van kapcsolva, narancssárga nyugalmi módban, és piros, ha ki van kapcsolva vagy nem elérhető (szürke: még nem található). A nyugalmi mód csak akkor ismerhető fel, ha a konzol közben az internethez kapcsolódva marad.
- **A PS5 felébresztése:** küldjön Wake-on-LAN csomagot, vagy nyomja meg röviden a **BOOT** gombot. A LED fehéren felvillan, a kísérlet alatt türkizen villog, majd **3× zölden** (a konzol válaszolt) vagy **3× pirosan** (nincs válasz) villan.
- A kísérletek között legalább 30 másodperc telik el; a figyelmen kívül hagyott kérést egy narancssárga villanás jelzi.

## A BOOT gomb

| Nyomva tartás | LED nyomva tartás közben | Eredmény |
|---|---|---|
| 1 s alatt | – | PS5 felébresztése |
| 1–5 s | fehér | semmi (megszakítás) |
| 5–20 s, majd elengedés | piros (15 s-tól villog) | a beállítópanel megnyitása |
| 20 s | lila, gyorsan villog | minden beállítás törlése és újraindítás |

Ha a tápellátás csatlakoztatásakor nyomva tartja a BOOT gombot, csökkentett mód indul Wi-Fi és Bluetooth nélkül (sárga LED).

## LED-színek

| Szín | Viselkedés | Jelentés |
|---|---|---|
| lila | folyamatos | nincs beállítva – a beállítópanel be van kapcsolva |
| kék | lassan villog | csatlakozás a Wi-Fi-hez |
| narancssárga | lassan villog | a Wi-Fi be van állítva, de 30 s óta nincs kapcsolat |
| zöld (halvány) | folyamatos | kész (a panelen kikapcsolható) |
| piros | folyamatos | a beállítópanel be van kapcsolva |
| sárga | folyamatos | csökkentett mód |
| fehér | 1 villanás | Wake-on-LAN érkezett vagy BOOT lenyomva |
| türkiz | gyorsan villog | ébresztési kísérlet folyamatban |
| zöld | 3 villanás | a konzol válaszolt |
| piros | 3 villanás | a konzol nem válaszolt |
| narancssárga | 1 villanás | kérés figyelmen kívül hagyva (kísérlet folyamatban vagy 30 s szünet) |

## A beállítópanel

- A panel és hálózata mentés után, a(z) **Bezárás módosítás nélkül** után vagy 10 perc tétlenség után kikapcsol. A(z) **Hosszabbítás** több időt ad.
- **A LED teljes kikapcsolása**: Semmilyen szín, villogás vagy villanás. A LED csak a BOOT lenyomva tartása közben mutatja továbbra is a nyomva tartás idejét.
- **Eszköz neve (nem kötelező)**: Pl. Nappali. A panel címében (`nappali.local`), a Home Assistantban és az oldal címében jelenik meg – több eszköz esetén hasznos.
- **Hozzáférés az otthoni hálózatról**: Alapértelmezetten bekapcsolva. Az oldal folyamatosan elérhető marad az otthoni hálózaton a `<ESP32 IP-címe>` és a `ps5-wake-xxxx.local` címen – bejelentkezés a panel jelszavával (felhasználónév: admin, ha nem módosította). A panel Wi-Fi hálózata nem indul el; megnyitásához tartsa nyomva a BOOT gombot 5 mp-ig.
- **Titkosított kapcsolat (HTTPS)**: Ajánlott. Az ESP32 saját tanúsítványt hoz létre, ezért az első látogatáskor a böngésző figyelmeztet, hogy a kapcsolat nem privát – hasonlítsa össze az alábbi ujjlenyomatot, és folytassa. HTTPS nélkül a jelszó és a beállítások titkosítatlanul haladnak át a hálózaton. A tanúsítvány lejárat előtt automatikusan megújul (az ESP32 az internetről veszi az időt); a(z) **Új tanúsítvány** azonnal újat készít.
- A(z) **PS5 felébresztése most** gombbal közvetlenül a panelről tesztelheti az ébresztést.

## Hibaelhárítás

- **A LED narancssárgán villog:** rossz Wi-Fi-jelszó, csak 5 GHz-es hálózat vagy gyenge jel. Nyissa meg a panelt, ellenőrizze a Wi-Fi-t, és vigye közelebb az ESP32-t a routerhez. Az ESP32 magától újraindul, ha 2 percig nincs kapcsolata.
- **A Wake-on-LAN elküldése után semmi sem történik (nincs fehér villanás):** a csomag nem jut el az ESP32-höz. Használja az ESP32 MAC-címét, a broadcast címet és a 9-es portot; ellenőrizze, hogy az ESP32 nem vendéghálózaton van-e.
- **Piros villanások egy kísérlet után:** ellenőrizze a Bluetooth-címeket (a kontroller újrapárosítása után olvassa ki őket újra), és vigye közelebb az ESP32-t a konzolhoz.
- **Az olvasó nem találja a kontrollert:** használjon adatkábelt (ne csak töltőkábelt), és a kontrollert közvetlenül a számítógéphez csatlakoztassa.
- **A böngésző figyelmeztet a tanúsítványra:** ez várható – a panelt az ESP32 saját tanúsítványa titkosítja. Lépjen tovább az oldalra; a panel megjeleníti a tanúsítvány ujjlenyomatát.

# PS5 Wake – användarhandbok

PS5 Wake är ett litet ESP32-C6-kort som slår på din PlayStation 5 när det tar emot ett Wake-on-LAN-paket eller när du trycker på dess knapp. Via Bluetooth utger det sig för att vara din parkopplade DualSense-handkontroll – precis som ett tryck på PS-knappen. Det fungerar från viloläge och från en helt avstängd konsol.

## Det här behöver du

- Ett ESP32-C6-kort med PS5 Wake (testat: ESP32-C6-DevKitM-1), som drivs av valfri USB-laddare.
- En DualSense- (eller DualSense Edge-)handkontroll som redan är parkopplad med din PS5.
- En dator och en USB-**datakabel** – bara en gång, för att installera firmware och läsa av handkontrollens adresser.
- Ett Wi-Fi-nätverk på 2,4 GHz.

## 1. Installera firmware

Öppna webbinstallationsprogrammet (länk i projektets README) i Chrome eller Edge, anslut kortet med en USB-kabel och klicka på **Install or update**. En ny enhet lyser **lila**. Det startar inställningspanelens nätverk av sig själv.

## 2. Öppna inställningspanelen

1. Håll knappen **BOOT** intryckt i 5–20 sekunder och släpp den. Lysdioden lyser **rött**. På en ny enhet är panelen redan på (lila lysdiod) – hoppa över det här steget.
2. Anslut datorn som du ska koppla handkontrollen till till Wi-Fi-nätverket `PS5-WAKE-xxxx`, lösenord `ps5wol-setup`.
3. Öppna **`https://192.168.4.1`** (eller `https://ps5-wake-xxxx.local`) i webbläsaren. Webbläsaren varnar för att anslutningen inte är privat, eftersom ESP32 använder ett eget certifikat – välj **Avancerat → Fortsätt** (namnen beror på webbläsaren).
4. Välj språk högst upp på sidan.

## 3. Läs av handkontrollens adresser

1. Anslut DualSense till datorn med USB-kabeln.
2. Ladda ned adressläsaren i avsnittet **Bluetooth** i panelen:
   - **Windows:** `ps5-wake-reader.cmd` – dubbelklicka på den. Om Windows varnar för en okänd utgivare väljer du **Mer information → Kör ändå**.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Läsaren visar konsolens och handkontrollens Bluetooth-adresser och kopierar en **inställningskod** (`PS5WAKE:…`) till urklipp. Klistra in den i fältet **Inställningskod från adressläsaren** – båda adresserna fylls i.

Adresserna ändras bara om du parkopplar handkontrollen med konsolen igen.

## 4. Slutför inställningen

1. Välj ditt Wi-Fi-nätverk med **Sök**, ange lösenordet och välj land.
2. Ange **Nytt lösenord för panelens nätverk** (minst 8 tecken). Det är obligatoriskt eftersom standardlösenordet är offentligt känt.
3. Klicka på **Spara och starta om**. Panelen stängs, lysdioden blinkar **blått** under anslutningen och lyser **grönt** när allt är klart.

## 5. Ställ in Wake-on-LAN

- Använd **MAC-adress för denna ESP32** från panelen, UDP-port **9** och nätverkets **broadcastadress** (t.ex. `192.168.1.255`) i din Wake-on-LAN-app (eller på routerns WoL-sida).
- Reservera en fast IP-adress för ESP32 i routern.
- Om appen eller routern redan använder **PS5:ans egen nätverks-MAC-adress** kan du ange den i fältet **Reagera även på PS5:s egen MAC-adress** – ESP32 reagerar då även på den. I mesh-nätverk (t.ex. ASUS AiMesh) kanske sådana paket inte når ESP32; skicka i så fall WoL till ESP32:s MAC-adress.

## Dagligt användande

- **Konsolens läge:** pricken på strömikonen högst upp i panelen är grön när PS5 är på, orange i viloläge och röd när den är av eller inte går att nå (grå: inte hittad än). Viloläge upptäcks bara om konsolen förblir ansluten till internet.
- **Väck PS5:** skicka ett Wake-on-LAN-paket eller tryck kort på **BOOT**. Lysdioden blinkar vitt en gång, blinkar turkost under försöket och sedan **3× grönt** (konsolen svarade) eller **3× rött** (inget svar).
- Det går minst 30 sekunder mellan försöken; en ignorerad begäran ger ett orange blink.

## Knappen BOOT

| Hålltid | Lysdiod medan du håller | Resultat |
|---|---|---|
| under 1 s | – | väck PS5 |
| 1–5 s | vit | inget (avbryt) |
| 5–20 s, släpp sedan | röd (blinkar från 15 s) | öppna inställningspanelen |
| 20 s | lila, blinkar snabbt | radera alla inställningar och starta om |

Om du håller BOOT intryckt när strömmen ansluts startar ett felsäkert läge utan Wi-Fi och Bluetooth (gul lysdiod).

## Lysdiodens färger

| Färg | Beteende | Betydelse |
|---|---|---|
| lila | lyser | inte inställd – inställningspanelen är på |
| blå | blinkar långsamt | ansluter till Wi-Fi |
| orange | blinkar långsamt | Wi-Fi inställt men ingen anslutning på över 30 s |
| grön (dämpad) | lyser | redo (kan stängas av i panelen) |
| röd | lyser | inställningspanelen är på |
| gul | lyser | felsäkert läge |
| vit | ett blink | Wake-on-LAN mottaget eller BOOT intryckt |
| turkos | blinkar snabbt | väckningsförsök pågår |
| grön | tre blink | konsolen svarade |
| röd | tre blink | konsolen svarade inte |
| orange | ett blink | begäran ignorerad (försök pågår eller 30 s paus) |

## Inställningspanelen

- Panelen och dess nätverk stängs av efter att du sparat, efter **Stäng utan ändringar** eller efter 10 minuter utan aktivitet. **Förläng** ger mer tid.
- **Stäng av lysdioden helt**: Inga färger, blinkningar eller blixtar. Bara medan du håller BOOT intryckt visar lysdioden fortfarande hålltiden.
- **Enhetsnamn (valfritt)**: T.ex. Vardagsrum. Används i panelens adress (`vardagsrum.local`), i Home Assistant och i sidans titel – praktiskt med flera enheter.
- **Åtkomst från hemnätverket**: På som standard. Sidan är alltid tillgänglig i hemnätverket på `<ESP32:s IP>` och `ps5-wake-xxxx.local` – logga in med panelens lösenord (användarnamn admin om du inte har ändrat det). Panelens Wi-Fi-nätverk startas inte; håll BOOT i 5 s för att öppna det.
- **Krypterad anslutning (HTTPS)**: Rekommenderas. ESP32 skapar ett eget certifikat, så vid första besöket varnar webbläsaren för att anslutningen inte är privat – jämför fingeravtrycket nedan och fortsätt. Utan HTTPS skickas lösenord och inställningar okrypterat i nätverket. Certifikatet förnyas automatiskt innan det går ut (ESP32 hämtar tiden från internet); **Nytt certifikat** skapar ett nytt direkt.
- **Väck PS5 nu** testar väckningen direkt från panelen.

## Felsökning

- **Lysdioden blinkar orange:** fel Wi-Fi-lösenord, endast ett 5 GHz-nätverk eller svag signal. Öppna panelen, kontrollera Wi-Fi och flytta ESP32 närmare routern. ESP32 startar om av sig själv om den är offline i 2 minuter.
- **Inget händer när du skickar Wake-on-LAN (inget vitt blink):** paketet når inte ESP32. Använd ESP32:s MAC-adress, broadcastadressen och port 9; kontrollera att ESP32 inte ligger på ett gästnätverk.
- **Röda blink efter ett försök:** kontrollera Bluetooth-adresserna (läs av dem igen efter ny parkoppling av handkontrollen) och flytta ESP32 närmare konsolen.
- **Läsaren hittar ingen handkontroll:** använd en datakabel (inte bara en laddkabel) och anslut handkontrollen direkt till datorn.
- **Webbläsaren varnar för certifikatet:** det är väntat – panelen krypteras med ESP32:s eget certifikat. Fortsätt till sidan; panelen visar certifikatets fingeravtryck.

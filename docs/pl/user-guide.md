# PS5 Wake – instrukcja użytkownika

PS5 Wake to mała płytka ESP32-C6, która włącza PlayStation 5 po odebraniu pakietu Wake-on-LAN albo po naciśnięciu
przycisku na płytce. Przez Bluetooth podaje się za sparowanego pada DualSense – tak jak naciśnięcie przycisku PS.
Działa z trybu spoczynku i z całkowicie wyłączonej konsoli.

## Co jest potrzebne

- Płytka ESP32-C6 z PS5 Wake (testowana: ESP32-C6-DevKitM-1), zasilana z dowolnej ładowarki USB.
- Pad DualSense (lub DualSense Edge) sparowany z PS5.
- Komputer i kabel USB **do transmisji danych** – tylko raz, do instalacji firmware i odczytu adresów pada.
- Sieć Wi-Fi 2,4 GHz.

## 1. Zainstaluj firmware

Otwórz instalator w przeglądarce (link w README projektu) w Chrome lub Edge, podłącz płytkę kablem USB i kliknij
**Install or update**. Nowe urządzenie świeci na **fioletowo**. Od razu uruchamia sieć panelu konfiguracyjnego.

## 2. Otwórz panel konfiguracyjny

1. Przytrzymaj przycisk **BOOT** przez 5–20 sekund i puść. Dioda zaświeci na **czerwono**. Nowe urządzenie ma panel włączony od razu (fioletowa dioda) – pomiń ten krok.
2. Na komputerze, do którego podłączysz pada, połącz się z siecią Wi-Fi **`PS5-WAKE-xxxx`**, hasło
   **`ps5wol-setup`**.
3. W przeglądarce otwórz **`https://192.168.4.1`** (albo `https://ps5-wake-xxxx.local`). Przeglądarka ostrzeże, że połączenie nie jest prywatne, bo ESP32 używa własnego certyfikatu – wybierz **Zaawansowane → Przejdź dalej** (nazwy zależą od przeglądarki).
4. Wybierz język na górze strony.

## 3. Odczytaj adresy pada

1. Podłącz pada DualSense do komputera kablem USB.
2. W sekcji **Bluetooth** panelu pobierz program do odczytu adresów:
   - **Windows:** `ps5-wake-reader.cmd` – uruchom dwuklikiem. Jeśli Windows ostrzeże o nieznanym wydawcy, wybierz
     **Więcej informacji → Uruchom mimo to**.
   - **macOS:** uruchom `pip3 install hidapi`, a potem `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (bez dodatkowych pakietów).
3. Program pokaże adresy Bluetooth konsoli i pada oraz skopiuje do schowka **kod konfiguracyjny** (`PS5WAKE:…`).
   Wklej go w pole **Kod konfiguracyjny z programu do odczytu** – oba adresy uzupełnią się same.

Adresy zmieniają się tylko po ponownym sparowaniu pada z konsolą.

## 4. Dokończ konfigurację

1. Wybierz sieć Wi-Fi przyciskiem **Skanuj**, wpisz jej hasło i wybierz kraj.
2. Wpisz **nowe hasło sieci panelu** (co najmniej 8 znaków). Jest wymagane, bo hasło domyślne jest publicznie znane.
3. Kliknij **Zapisz i uruchom ponownie**. Panel się zamknie, dioda będzie migać na **niebiesko** podczas łączenia
   i zaświeci na **zielono**, gdy wszystko będzie gotowe.

## 5. Skonfiguruj Wake-on-LAN

- W aplikacji Wake-on-LAN (albo na stronie WoL routera) użyj **adresu MAC ESP32** pokazanego w panelu, portu UDP
  **9** i **adresu rozgłoszeniowego** sieci (np. `192.168.1.255`).
- Zarezerwuj w routerze stały adres IP dla ESP32.
- Jeśli aplikacja lub router używają już **sieciowego MAC samej PS5**, możesz go wpisać w panelu – ESP32 będzie
  reagować także na niego. W sieciach mesh (np. ASUS AiMesh) takie pakiety mogą nie docierać do ESP32; wtedy
  wysyłaj WoL na MAC ESP32.

## Codzienne używanie

- **Stan konsoli:** kropka przy ikonie zasilania na górze panelu jest zielona, gdy PS5 jest włączona, pomarańczowa w trybie spoczynku i czerwona, gdy jest wyłączona lub niedostępna (szara: jeszcze nie wykryta). Tryb spoczynku jest wykrywany tylko wtedy, gdy konsola pozostaje w nim połączona z internetem.
- **Wybudzenie PS5:** wyślij pakiet Wake-on-LAN albo krótko naciśnij **BOOT**. Dioda błyśnie na biało, podczas
  próby miga na turkusowo, a potem błyska **3× na zielono** (konsola odpowiedziała) albo **3× na czerwono** (brak
  odpowiedzi).
- Między próbami mija co najmniej 30 sekund; zignorowane żądanie sygnalizuje jeden pomarańczowy błysk.

## Przycisk BOOT

| Czas trzymania | Dioda w trakcie | Efekt |
|---|---|---|
| poniżej 1 s | – | wybudzenie PS5 |
| 1–5 s | biała | nic (anulowanie) |
| 5–20 s, potem puszczenie | czerwona (od 15 s miga) | otwarcie panelu |
| 20 s | fioletowa, szybko miga | skasowanie ustawień i restart |

Przytrzymanie BOOT przy podłączaniu zasilania uruchamia tryb awaryjny bez Wi-Fi i Bluetooth (żółta dioda).

## Kolory diody

| Kolor | Zachowanie | Znaczenie |
|---|---|---|
| fioletowy | świeci | brak konfiguracji – panel konfiguracyjny włączony |
| niebieski | miga wolno | łączenie z Wi-Fi |
| pomarańczowy | miga wolno | Wi-Fi ustawione, ale brak połączenia ponad 30 s |
| zielony (przyciemniony) | świeci | gotowy (można wyłączyć w panelu) |
| czerwony | świeci | panel konfiguracyjny włączony |
| żółty | świeci | tryb awaryjny |
| biały | 1 błysk | odebrano Wake-on-LAN lub naciśnięto BOOT |
| turkusowy | miga szybko | trwa próba wybudzenia |
| zielony | 3 błyski | konsola odpowiedziała |
| czerwony | 3 błyski | konsola nie odpowiedziała |
| pomarańczowy | 1 błysk | żądanie zignorowane (trwa próba lub 30 s przerwy) |

## Panel konfiguracyjny

- Panel i jego sieć wyłączają się po zapisaniu, po **Zamknij panel bez zmian** albo po 10 minutach bez żadnej
  czynności. Przycisk **Przedłuż (+10 min)** daje więcej czasu.
- **Wyłącz diodę całkowicie**: Żadnych kolorów, mignięć ani błysków. Tylko podczas trzymania BOOT dioda nadal pokazuje czas przytrzymania.
- **Nazwa urządzenia (opcjonalnie)**: Np. Salon. Używana w adresie panelu (`salon.local`), w Home Assistant i w tytule strony – przydatne przy kilku urządzeniach.
- **Dostęp z sieci domowej**: Domyślnie włączone. Strona będzie stale dostępna w sieci domowej pod adresem `<IP ESP32>` oraz `ps5-wake-xxxx.local` – logowanie hasłem panelu (nazwa użytkownika admin, jeśli jej nie zmieniono). Sieć Wi-Fi panelu nie jest uruchamiana; aby ją otworzyć, przytrzymaj BOOT 5 s.
- **Szyfrowane połączenie (HTTPS)**: Zalecane. ESP32 tworzy własny certyfikat, więc przy pierwszej wizycie przeglądarka ostrzeże, że połączenie nie jest prywatne – porównaj odcisk poniżej i kontynuuj. Bez HTTPS hasło i ustawienia są przesyłane w sieci bez szyfrowania. Certyfikat jest odnawiany automatycznie przed wygaśnięciem (ESP32 pobiera czas z internetu); **Nowy certyfikat** tworzy nowy od razu.
- **Wybudź PS5 teraz** pozwala przetestować wybudzanie z panelu.

## Rozwiązywanie problemów

- **Dioda miga na pomarańczowo:** złe hasło Wi-Fi, sieć tylko 5 GHz albo słaby sygnał. Otwórz panel, sprawdź
  ustawienia Wi-Fi i przysuń ESP32 bliżej routera. ESP32 sam się zrestartuje, jeśli przez 2 minuty nie ma
  połączenia.
- **Po wysłaniu Wake-on-LAN nic się nie dzieje (brak białego błysku):** pakiet nie dociera do ESP32. Użyj MAC ESP32,
  adresu rozgłoszeniowego i portu 9; sprawdź, czy ESP32 nie jest w sieci dla gości.
- **Czerwone błyski po próbie wybudzenia:** sprawdź adresy Bluetooth (odczytaj je ponownie po ponownym sparowaniu
  pada) i przysuń ESP32 bliżej konsoli.
- **Program nie widzi pada:** użyj kabla do transmisji danych (nie tylko ładującego) i podłącz pada bezpośrednio do
  komputera.
- **Przeglądarka ostrzega o certyfikacie:** to normalne – panel jest szyfrowany własnym certyfikatem ESP32. Przejdź dalej do strony; panel pokazuje odcisk certyfikatu.

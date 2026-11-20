# PS5 Wake – kullanım kılavuzu

PS5 Wake, bir Wake-on-LAN paketi aldığında veya düğmesine bastığınızda PlayStation 5'inizi açan küçük bir ESP32-C6 kartıdır. Bluetooth üzerinden eşleştirilmiş DualSense kumandanız gibi davranır – tıpkı PS düğmesine basmak gibi. Dinlenme modundan ve tamamen kapalı konsoldan çalışır.

## Gerekenler

- PS5 Wake yüklü bir ESP32-C6 kartı (test edilen: ESP32-C6-DevKitM-1), herhangi bir USB şarj cihazından beslenir.
- PS5'inizle zaten eşleştirilmiş bir DualSense (veya DualSense Edge) kumanda.
- Bir bilgisayar ve **veri aktarımı destekleyen** bir USB kablo – yalnızca bir kez, yazılımı yüklemek ve kumanda adreslerini okumak için.
- 2,4 GHz Wi-Fi ağı.

## 1. Yazılımı yükleyin

Web yükleyicisini (proje README'sindeki bağlantı) Chrome veya Edge'de açın, kartı USB kabloyla bağlayın ve **Install or update**'a tıklayın. Yeni bir cihaz **mor** yanar. Kurulum paneli ağını kendiliğinden başlatır.

## 2. Kurulum panelini açın

1. **BOOT** düğmesini 5–20 saniye basılı tutun ve bırakın. LED **kırmızı** yanar. Yeni bir cihazda panel zaten açıktır (mor LED) – bu adımı atlayın.
2. Kumandayı bağlayacağınız bilgisayarda `PS5-WAKE-xxxx` Wi-Fi ağına bağlanın, şifre `ps5wol-setup`.
3. Tarayıcıda **`https://192.168.4.1`** (veya `https://ps5-wake-xxxx.local`) adresini açın. ESP32 kendi sertifikasını kullandığı için tarayıcı bağlantının gizli olmadığı uyarısını verir – **Gelişmiş → Devam et**'i seçin (adlar tarayıcıya göre değişir).
4. Sayfanın üst kısmından dilinizi seçin.

## 3. Kumanda adreslerini okuyun

1. DualSense'i USB kabloyla bilgisayara bağlayın.
2. Panelin **Bluetooth** bölümünden adres okuyucuyu indirin:
   - **Windows:** `ps5-wake-reader.cmd` – çift tıklayarak çalıştırın. Windows bilinmeyen yayıncı uyarısı verirse **Ek bilgi → Yine de çalıştır**'ı seçin.
   - **macOS:** `pip3 install hidapi` → `python3 ps5_wake_reader.py`
   - **Linux:** `sudo python3 ps5_wake_reader.py`
3. Okuyucu konsolun ve kumandanın Bluetooth adreslerini gösterir ve bir **kurulum kodunu** (`PS5WAKE:…`) panoya kopyalar. Bunu **Adres okuyucudan kurulum kodu** alanına yapıştırın – her iki adres de doldurulur.

Adresler yalnızca kumandayı konsolla yeniden eşleştirirseniz değişir.

## 4. Kurulumu tamamlayın

1. **Tara** ile Wi-Fi ağınızı seçin, şifresini girin ve ülkenizi seçin.
2. **Panel ağı için yeni şifre** girin (en az 8 karakter). Varsayılan şifre herkesçe bilindiği için zorunludur.
3. **Kaydet ve yeniden başlat**'a tıklayın. Panel kapanır, LED bağlanırken **mavi** yanıp söner ve her şey hazır olduğunda **yeşil** yanar.

## 5. Wake-on-LAN'ı ayarlayın

- Wake-on-LAN uygulamanızda (veya yönlendiricinin WoL sayfasında) paneldeki **Bu ESP32’nin MAC adresi** değerini, UDP **9** numaralı bağlantı noktasını ve ağınızın **yayın adresini** (ör. `192.168.1.255`) kullanın.
- Yönlendiricide ESP32 için sabit bir IP adresi ayırın.
- Uygulamanız veya yönlendiriciniz zaten **PS5'in kendi ağ MAC adresini** kullanıyorsa, bunu **PS5’in kendi MAC adresine de tepki ver** alanına girebilirsiniz – ESP32 ona da tepki verir. Mesh ağlarda (ör. ASUS AiMesh) bu paketler ESP32'ye ulaşmayabilir; bu durumda WoL'u ESP32'nin MAC adresine gönderin.

## Günlük kullanım

- **Konsol durumu:** panelin üstündeki güç simgesindeki nokta PS5 açıkken yeşil, dinlenme modunda turuncu, kapalıyken veya erişilemezken kırmızıdır (gri: henüz bulunamadı). Dinlenme modu yalnızca konsol internete bağlı kalırsa algılanır.
- **PS5'i uyandırma:** bir Wake-on-LAN paketi gönderin veya **BOOT**'a kısa basın. LED bir kez beyaz yanıp söner, deneme sırasında camgöbeği yanıp söner, ardından **3× yeşil** (konsol yanıt verdi) veya **3× kırmızı** (yanıt yok) yanıp söner.
- Denemeler arasında en az 30 saniye geçer; yok sayılan bir istek tek bir turuncu yanıp sönmeyle gösterilir.

## BOOT düğmesi

| Basılı tutma süresi | Basılıyken LED | Sonuç |
|---|---|---|
| 1 sn'den az | – | PS5'i uyandırma |
| 1–5 sn | beyaz | hiçbir şey (iptal) |
| 5–20 sn, sonra bırakma | kırmızı (15 sn'den itibaren yanıp söner) | kurulum panelini açma |
| 20 sn | mor, hızlı yanıp söner | tüm ayarları silme ve yeniden başlatma |

Güç bağlanırken BOOT basılı tutulursa Wi-Fi ve Bluetooth olmadan güvenli mod başlar (sarı LED).

## LED renkleri

| Renk | Davranış | Anlamı |
|---|---|---|
| mor | sabit | yapılandırılmamış – kurulum paneli açık |
| mavi | yavaş yanıp söner | Wi-Fi'ye bağlanıyor |
| turuncu | yavaş yanıp söner | Wi-Fi ayarlı ama 30 sn'den uzun süredir bağlı değil |
| yeşil (soluk) | sabit | hazır (panelden kapatılabilir) |
| kırmızı | sabit | kurulum paneli açık |
| sarı | sabit | güvenli mod |
| beyaz | bir kez | Wake-on-LAN alındı veya BOOT'a basıldı |
| camgöbeği | hızlı yanıp söner | uyandırma denemesi sürüyor |
| yeşil | üç kez | konsol yanıt verdi |
| kırmızı | üç kez | konsol yanıt vermedi |
| turuncu | bir kez | istek yok sayıldı (deneme sürüyor veya 30 sn bekleme) |

## Kurulum paneli

- Panel ve ağı kaydettikten sonra, **Değişiklik yapmadan kapat** sonrasında veya 10 dakika işlem yapılmazsa kapanır. **Uzat** daha fazla süre verir.
- **LED'i tamamen kapat**: Hiçbir renk, yanıp sönme veya parlama olmaz. Yalnızca BOOT'u basılı tutarken LED basılı tutma süresini göstermeye devam eder.
- **Cihaz adı (isteğe bağlı)**: Örn. Salon. Panel adresinde (`salon.local`), Home Assistant'ta ve sayfa başlığında kullanılır – birden fazla cihaz varsa faydalıdır.
- **Ev ağından erişim**: Varsayılan olarak açık. Sayfa ev ağınızda `<ESP32 IP adresi>` ve `ps5-wake-xxxx.local` adreslerinde sürekli erişilebilir kalır – panel şifresiyle giriş yapın (değiştirmediyseniz kullanıcı adı admin). Panel Wi-Fi ağı başlatılmaz; açmak için BOOT'u 5 sn basılı tutun.
- **Şifreli bağlantı (HTTPS)**: Önerilir. ESP32 kendi sertifikasını oluşturur; bu nedenle ilk ziyarette tarayıcı bağlantının gizli olmadığı uyarısını gösterir – aşağıdaki parmak izini karşılaştırıp devam edin. HTTPS olmadan şifre ve ayarlar ağda şifrelenmeden iletilir. Sertifika süresi dolmadan otomatik olarak yenilenir (ESP32 saati internetten alır); **Yeni sertifika** hemen yenisini oluşturur.
- **PS5’i şimdi uyandır** uyandırmayı doğrudan panelden test eder.

## Sorun giderme

- **LED turuncu yanıp sönüyor:** yanlış Wi-Fi şifresi, yalnızca 5 GHz ağ veya zayıf sinyal. Paneli açın, Wi-Fi ayarlarını kontrol edin ve ESP32'yi yönlendiriciye yaklaştırın. ESP32, 2 dakika bağlantısız kalırsa kendiliğinden yeniden başlar.
- **Wake-on-LAN gönderildikten sonra hiçbir şey olmuyor (beyaz yanıp sönme yok):** paket ESP32'ye ulaşmıyor. ESP32'nin MAC adresini, yayın adresini ve 9 numaralı bağlantı noktasını kullanın; ESP32'nin misafir ağında olmadığını kontrol edin.
- **Denemeden sonra kırmızı yanıp sönmeler:** Bluetooth adreslerini kontrol edin (kumandayı yeniden eşleştirdiyseniz tekrar okuyun) ve ESP32'yi konsola yaklaştırın.
- **Okuyucu kumandayı bulamıyor:** veri kablosu kullanın (yalnızca şarj kablosu değil) ve kumandayı doğrudan bilgisayara bağlayın.
- **Tarayıcı sertifika uyarısı veriyor:** bu beklenen bir durumdur – panel ESP32'nin kendi sertifikasıyla şifrelenir. Sayfaya devam edin; panel sertifika parmak izini gösterir.

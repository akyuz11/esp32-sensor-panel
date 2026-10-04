# Deney Kartı Android uygulaması

Android 8+ için yerel BLE uygulaması. S24 Ultra dahil BLE destekli telefonlarda
uygulama içinden kart seçilir; sistem Bluetooth ayarlarından önceden eşleştirme
gerekmez. Wi-Fi/internet izni yoktur.

## Kullanım

1. BLE destekli firmware'i ESP32-S3'e USB üzerinden yükleyin: pio run -t upload.
2. app/build/outputs/apk/debug/app-debug.apk dosyasını telefona aktarın ve açın.
3. Android kurulum sırasında isterse dosyayı açtığınız uygulamaya kurulum izni verin.
4. Deney Kartı uygulamasında Yakındaki cihazlar iznini verin.
5. Kartı bul → adresiyle gösterilen Deney-Karti cihazını seçin.
6. Kartı eğin. Su terazisi, ivme, dönüş hızı ve pil gerilimi güncellenir.
7. Bu konumu sıfır kabul et düğmesi yalnızca telefondaki eğim göstergesini sıfırlar.

Android 8–11'de BLE taraması konum izni ve Konum hizmetinin açık olmasını
gerektirebilir. Android 12+ için Yakındaki cihazlar izni kullanılır.
Uygulama arka plana geçince bağlantıyı kapatır. Yeniden açınca Kartı bul ile
bağlanın. Kopma sonrası da aynı düğmeyle yeniden bağlanılır.

## Veriler ve sınırlar

- Kartın gerçek ivmeölçeri (g), jiroskobu (derece/s), pil ölçümü (V).
- Kartın açılışından itibaren milisaniye; bu bir takvim zaman damgası değildir.
- Sıcaklık/nem/MQTT değerleri bu ilk sürümde aktarılmaz.
- Pil algılanmazsa ölçüm yok gösterilir.
- Yaklaşık 5 Hz; Driver_Loop içindeki diğer ağ/SD işlemleri güncellemeyi geciktirebilir.
- 3 saniye veri gelmezse son değerlerin güncel olmadığı belirtilir.
- Firmware'deki mevcut jiroskop aralığı ±64 derece/s.
- Tek telefonla sınıf denemesi için hazırlanmıştır. BLE bağı şifreli/bonded değildir;
  yakın çevredeki bir BLE istemcisi ölçümlere abone olabilir.
- SD dosyalarını indirme veya telefondan kayıt komutu bu sürümde yoktur.

## Derleme

Android Studio ile bu klasörü açın (JDK 17, SDK 35).
Bu bilgisayarda proje içine araçlar kurulduktan sonra:
powershell -ExecutionPolicy Bypass -File android-app/build.ps1

AGP 8.7.3 / Gradle 8.9, compile/target SDK 35, minimum SDK 26.
Debug APK yerel geliştirme anahtarıyla imzalanır; Play Store yayını değildir.

## BLE protokolü v1

Service: 87ba0001-6d31-4b6e-8bd0-ef38247a9100
Notify: 87ba0002-6d31-4b6e-8bd0-ef38247a9100
CCCD: 00002902-0000-1000-8000-00805f9b34fb

Her bildirim 20 bayttır. Çok baytlı alanlar little endian:
0: sürüm 1
1: bayraklar, bit 0 pil geçerli
2..5: uint32 uptime ms
6..11: int16 X/Y/Z ivme, 1000 ile bölünür (g)
12..17: int16 X/Y/Z dönüş hızı, 100 ile bölünür (derece/s)
18..19: uint16 pil mV, 1000 ile bölünür (V)

En küçük BLE ATT MTU'suna sığar; MTU pazarlığı gerekmez.
PacketTest, işaretli sayıları, zaman sayacı taşma yorumunu, açı hesabını,
pil geçerliliğini ve bozuk/sürümü bilinmeyen paketlerin reddini kontrol eder.
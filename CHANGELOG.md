# Changelog

# CODLAI ERA (New Models)

## [Unreleased]

## [1.1.1] - 2026-10-09
### Fixed
- Ornekler Arduino IDE / arduino-cli'de "variable or field ... declared void" hatasi veriyordu: Arduino'nun otomatik prototipleri enum tanimindan once yaziliyordu. Enum parametreli fonksiyonlara elle prototip eklendi: IOTBOT_Armbot_and_Carbot_Wireless_Control. (PlatformIO'da derleniyordu.)

## [1.1.0] - 2026-10-07
### Added
- **Ornekler bastan yazildi (8 ornek):** hepsi ayni kurala uyuyor - en ustte `bool turkish` ile TR/EN secimi, calisirken seri porttan `dil`/`lang` ile degisim, iki dilli ve bloklamayan seri komutlar (`yardim`/`help`). Bir seyi suren ornekler OTOMATIK gosteriyle baslar, buton ile MANUEL moda gecilir.
- Ornekler `Klasor/Klasor.ino` yapisina tasindi: Arduino IDE *Dosya > Ornekler* menusunde hepsi gorunur. `library.json` "examples" alani glob kullaniyor.
- `examples/examples.json`: her ornegin yolu, karti, gereken moduller/ayarlar, TR/EN ozeti ve seri komutlari (editor.codlai.com "Kutuphane Ornekleri" ekrani icin; `scripts/generate_examples_json.py` ile uretilir).

### Fixed
- Desteklenmeyen platformda bos pin tanimlari anlasilmaz bir derleme hatasi veriyordu; artik net `#error` mesaji. Seri mesajlar TR/EN.

## [1.0.6] - 2026-09-29
### Fixed
- `IOTBOT_Armbot_and_Carbot_Wireless_Control.ino`: ARMBOT'taki kopya Aralik 2025'ten kalma, artik kullanilmayan eski bir veri yapisiyla yazilmisti ve var olmayan dosya adlarina yonlendiriyordu; CARBOT'taki guncel kopyayla esitlendi. Joystick X (ADC2) ESP-NOW ile okunamadigi icin govde donusu encoder'a tasindi - ayrintilar CODLAI_CARBOT 1.1.3.
- `MINIBOT_ARMBOT_ESP_NOW_Slave_Control.ino`: 500 ms'de bir "buradayim" sinyali (deviceType 4) gonderiyor; kumanda baglanti gostergesini buna gore ciziyor. Gelen acilar 0-180'e sinirlaniyor.
- `IOTBOT_Armbot_and_Carbot_Wired_Control.ino`: mod degisiminde pin devri (CARBOT'a donunce motorlar/direksiyon olu kaliyordu) ve magaza modu buton polaritesi duzeltildi - ayrintilar CODLAI_CARBOT 1.1.3.
- Depoda eski bir PlatformIO kurulum kaydi (`.piopm`, surum 1.2.6) izleniyordu ve GitHub'a da gidiyordu; kutuphaneyi GitHub'dan ya da yerel klasorden (symlink) kuran projelerde bagimlilik agaci yanlis surum gosteriyordu. Dosya kaldirildi ve `.gitignore`'a eklendi. (Duvar projesi oturumunun bulgusu.)
- `IOTBOT_Armbot_and_Carbot_Wired_Control.ino` derlenmiyordu ("'B12State' does not name a type"): `B12State`, `Mode`, `AppState` tipleri ilk fonksiyondan sonra tanimliydi, Arduino ise fonksiyon prototiplerini ilk fonksiyonun onune ekler. Tip tanimlari dosyanin basina tasindi.
- `examples/IOTBOT_Armbot_and_Carbot_Wired_Control/` klasorunde ikinci bir `.ino` (`... copy.ino`, Nisan'dan kalma eski deneme) duruyordu; Arduino IDE ayni klasordeki tum `.ino` dosyalarini birlikte derledigi icin `setup()`/`loop()` iki kez tanimlanip ornek acilamiyordu. Kopya kaldirildi (git gecmisinde duruyor).

### Added
- `MINIBOT_ARMBOT_ESP_NOW_Slave_Control.ino`: magaza modu (`action = 10`) - kumanda CARBOT'u kontrol ederken kol bloklamadan gosteri yapiyor (govde taramasi, omuz/dirsek, kiskac, el sallama) ve normal komut gelince birakiyor. Servolar hedefe sinirli hizla gidiyor (gosteri pozundan kontrole donerken ani sicrama yok). Ayrintilar CODLAI_CARBOT 1.1.3.
- Yeni ornek: `ARMBOT_RockPaperScissors_Game_Example.ino` - butona her basista buzzer ile "Tas, Kagit, Makas... Cektik!" sayimi yapip ardindan RASTGELE bir el pozu (yumruk/acik el/makas) sergileyen eglenceli, egitici oyun ornegi.

## [1.0.5] - 2026-09-25
### Fixed
- `begin()` artik ESP32 dalinda servo baglanti hatasini `attach()`'in donus degeri yerine `attached()` ile kontrol ediyor. ESP32Servo 3.x `attach()` basarida LEDC kanal numarasini dondurur (ilk servo icin 0); bu deger yanlislikla "hata" sayilip ilk eksen ("Axis 1") her zaman gercekte bagliyken bile "Servo attach failed!" basiyordu.

## [1.0.4] - 2025-12-18
### Updated
- Revamped the `IOTBOT_Armbot_and_Carbot_Wireless_Control.ino` walkthrough to cover the latest helper APIs for carrier-enabled control flows.
- Polished `MINIBOT_ARMBOT_ESP_NOW_Slave_Control.ino` so the ESP-NOW handshake steps match the refreshed IOTBOT/MINIBOT helpers.

## [1.0.3] - 2025-03-09
### Fixed
- PlatformIO yeniden yayını için sürüm numarası artırıldı.

## [1.0.0] - 2025-03-04
### Added
- **Rebranding**: Transitioned from CODROB to CODLAI.
- Standardized library structure.
- Added `serialStart` and `serialWrite` wrappers.
- Added `waveHand` function.
- Updated examples to use library wrappers.
- Initial Release for PlatformIO and Arduino IDE.

---

# CODROB ERA (Legacy Models)

## [1.2.6] - 2025-03-04
### Added
- Added Arduino IDE Suport

## [1.1.3] - 2025-01-23
### Fixed
- Resolved servo initialization issues for Axis 1 and Axis 2.
- Enhanced buzzer functionality for better tone generation.
- Improved motion accuracy and speed control across all axes.

### Added
- Detailed error logging during servo attachment.
- `moveToAngle` refined for precise movements and smoother transitions.

## [1.1.2] - 2025-01-22
### Fixed
- Corrected servo motion delays.
- Minor improvements in buzzer tone output.

## [1.1.1] - 2025-01-20
### Added
- New tone-based feedback using `buzzerPlay`.
- Smoother transition logic for servo movement.

## [1.1.0] - 2025-01-12
### Added
- Stability improvements for all axes.
- Buzzer functionality updated with tone control.

## [1.0.5] - 2025-01-08
### Added
- Ability to play Istiklal Marşı melody.
- New tones for the buzzer.

### Fixed
- Resolved servo motion delay issues.

## [1.0.4] - 2025-01-05
### Changed
- Enhanced library structure.
- Updated `ServoESP32` dependency.

## [1.0.3] - 2025-01-03
### Fixed
- Resolved buzzer test function issues.

## [1.0.2] - 2025-01-02
### Added
- Axis movement functions added.


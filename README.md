# ARMBOT Library User Guide
This library is specially designed and produced by the CODLAI developer team to control the ARMBOT product.
![alt text](<images/1.jpg>)

## Dependencies & Versions / Bağımlılıklar ve Sürümler

### Platform Versions
- **ESP8266**: 2.5.0 - 3.0.2
- **ESP32**: 1.0.6 - 2.0.14

### Library Dependencies
- **ESP32Servo**: ^1.1.0 (Only for ESP32 / Sadece ESP32 için)
- **Servo**: Built-in (For ESP8266 / ESP8266 için dahili)

---

## Using with Arduino IDE

### Installation

1. Open the Arduino IDE.
2. Go to "Sketch" -> "Include Library" -> "Manage Libraries..." from the menu bar.
3. Type "ARMBOT" in the search box.
4. Find the ARMBOT library and click the "Install" button to complete the installation.




# ARMBOT Kütüphanesi Kullanım Kılavuzu
Bu kütüphane CODLAI  geliştirici ekibi tarafından ARMBOT ürününü kontrol etmek için özel olarak tasarlanmış ve üretilmiştir. 
![alt text](<images/1.jpg>)



## Arduino IDE ile Kullanım

### Kurulum

1. Arduino IDE'yi açın.
2. Menu çubuğundan "Sketch" -> "Include Library" -> "Manage Libraries..." seçeneğine gidin.
3. Arama kutusuna "ARMBOT" yazın.
4. ARMBOT kütüphanesini bulun ve "Install" düğmesine tıklayarak kurulumu tamamlayın.




<!-- EXAMPLES:START -->
# Examples / Örnekler

**EN:** 8 examples - Robot (8). Every example follows the same rules:
- **Turkish / English:** `bool turkish = true;` at the top picks the language. Type `lang` (or `dil`) in the Serial Monitor to switch while it runs. Serial, LCD and web texts follow it.
- **Serial port (115200 baud):** commands work in both languages (`help` = `yardim`, `angle 90` = `aci 90` = `açı 90`) and with any line-ending setting. Type `help` for the list.
- **Auto / manual:** 5 examples that drive something (motor, servo, relay, LED, buzzer, robot) start in **AUTO** mode with a demo. Press **B3** on IOTBOT, **B1** (GPIO0) on MINIBOT to switch to **MANUAL**. On IOTBOT you drive the arm with the joystick/pot, on MINIBOT with serial commands (base 90, gripper open ...). An actuator command sent from serial also switches to manual.
- 0 examples need your own settings (WiFi, tokens, keys): fill in the `YOUR_...` placeholders.
- Each example is in its own folder (`Folder/Folder.ino`), so it shows up under *File > Examples* in the Arduino IDE.
- `examples/examples.json` lists every example with its board, required modules, summary (TR/EN) and serial commands (used by editor.codlai.com).

**TR:** 8 örnek - Robot (8). Tüm örnekler aynı kurallara uyar:
- **Türkçe / İngilizce:** En üstteki `bool turkish = true;` dili seçer. Çalışırken Seri Monitör'e `dil` (veya `lang`) yazarak değiştirebilirsiniz. Seri port, LCD ve web metinleri seçilen dili izler.
- **Seri port (115200 baud):** komutlar iki dilde de çalışır (`yardim` = `help`, `aci 90` = `açı 90` = `angle 90`) ve satır sonu ayarı ne olursa olsun algılanır. Komut listesi için `yardim` yazın.
- **Otomatik / manuel:** Bir şey süren 5 örnek (motor, servo, röle, LED, buzzer, robot) **OTOMATİK** modda bir gösteriyle başlar. IOTBOT'ta **B3**, MINIBOT'ta **B1** (GPIO0) butonu ile **MANUEL** moda geçersiniz. IOTBOT'ta kolu joystick/pot ile, MINIBOT'ta seri komutlarla (taban 90, kiskac ac ...) sürersiniz. Seri porttan gönderilen bir çalıştırma komutu da manuel moda geçirir.
- 0 örnek sizin ayarlarınızı ister (WiFi, token, anahtar): `YOUR_...` yer tutucularını doldurun.
- Her örnek kendi klasöründedir (`Klasör/Klasör.ino`); Arduino IDE'de *Dosya > Örnekler* menüsünde görünür.
- `examples/examples.json` her örneği kartı, gerektirdiği modüller, özeti (TR/EN) ve seri komutlarıyla listeler (editor.codlai.com kullanır).
<!-- EXAMPLES:END -->

# Library Structure & Contributing / Kütüphane Yapısı ve Katkıda Bulunma
This is a lightweight library.
- **Structure:** Simple class-based structure without complex configuration.
- **Extension:** Add new functions directly to the class. If adding heavy modules, consider refactoring to a modular design.

Bu hafif bir kütüphanedir.
- **Yapı:** Karmaşık yapılandırma gerektirmeyen basit sınıf tabanlı yapı.
- **Genişletme:** Yeni fonksiyonları doğrudan sınıfa ekleyin. Ağır modüller ekleyecekseniz, modüler bir tasarıma geçmeyi düşünün.

# Katkıda Bulunma / Contributing
Katkıda bulunmak isterseniz, lütfen GitHub deposuna Pull Request gönderin. / If you'd like to contribute, please send a Pull Request to the GitHub repository.

# Lisans / License
Bu kütüphane 2024 Yılında CODLAI Teknoloji tarafından lisanslanmıştır. Detaylar için LICENSE dosyasına bakınız. / Copyright (c) 2024 CODLAI Teknoloji. All right reserved.

 See the LICENSE file for details.

![alt text](<images/2.jpg>)
![alt text](<images/3.jpg>)
![alt text](<images/4.jpg>)
![alt text](<images/5.jpg>)

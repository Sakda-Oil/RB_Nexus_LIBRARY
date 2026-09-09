# RB_Nexus

แพ็กเกจบอร์ด RB_Nexus สำหรับ Arduino IDE / Boards Manager
ใช้ ESP32-WROOM-32 ตามสเปคฮาร์ดแวร์รุ่น 0.1 และ Arduino-ESP32 core 3.3.10

## ติดตั้งบนเครื่องอื่น

ใช้ลิงก์เดิมนี้เพื่อรับเวอร์ชันล่าสุดใน Additional Boards Manager URLs:

```text
https://raw.githubusercontent.com/Sakda-Oil/RB_Nexus_LIBRARY/main/package_RB_Nexus_index.json
```

1. เปิด Arduino IDE 2 แล้วเข้า Settings / Preferences
2. เพิ่มลิงก์ข้างต้นใน **Additional Boards Manager URLs** โดยเก็บลิงก์เดิมไว้
3. เปิด **Boards Manager** ค้นหา **RB_Nexus** และติดตั้งเวอร์ชัน 0.1.1
4. เลือก **Tools > Board > RB_Nexus > RB_Nexus**
5. เลือก **Tools > Port** ให้ตรงกับบอร์ด
6. เปิด **File > Examples > RB_Nexus > BoardInfo** แล้วอัปโหลด
7. เปิด Serial Monitor ที่ **115200 baud**

แพ็กเกจนี้มี core และไลบรารีมาตรฐานของ ESP32 รวมถึง RB_Nexus.h
เครื่องมือคอมไพล์จะดาวน์โหลดจากแหล่งเดิมของ Espressif โดยอัตโนมัติ
ไม่ต้องเพิ่มลิงก์ Friend Robot หรือติดตั้ง core ESP32 แยกก่อน
ต้องเชื่อมต่ออินเทอร์เน็ตระหว่างติดตั้งครั้งแรก

## ค่าเริ่มต้นและการเลือกบอร์ด

| รายการ | ค่าเริ่มต้น |
| --- | --- |
| ชื่อบอร์ด | RB_Nexus |
| MCU | ESP32 รุ่นดั้งเดิม / ESP32-WROOM-32 |
| CPU | 240 MHz |
| Flash | 4 MB, DIO, 40 MHz (ต้องตรวจให้ตรงกับโมดูลจริง) |
| PSRAM | Disabled |
| Upload Speed | 115200 |
| Partition | Default 4MB with SPIFFS |
| FQBN | RB_Nexus:esp32:rb_nexus |

ปรับ CPU, Flash Size, Flash Mode, Partition Scheme, PSRAM และ Upload Speed
ในเมนู Tools ได้ บอร์ด ESP32 เดิมจากแพ็กเกจอื่นยังเลือกใช้งานได้ตามปกติ
RB_Nexus รุ่นนี้ใช้เฉพาะ ESP32-WROOM-32; ESP32-S3/C3 เป็นคนละชิปและต้องมี
board definition ของรุ่นนั้นโดยเฉพาะ

## ขอบเขตรุ่น 0.1.1

รุ่นนี้เป็นแพ็กเกจเลือกบอร์ด คอมไพล์ และอัปโหลด พร้อมตัวอย่างพื้นฐานด้านล่าง
ยังไม่ได้ทดสอบการอัปโหลดกับฮาร์ดแวร์ RB_Nexus จริง
ยังไม่มีผังขามอเตอร์ เอ็นโค้ดเดอร์ ADC/PWM CAN I2C หรือ IMU
จึงยังไม่มี API ควบคุมอุปกรณ์เหล่านั้น ดู [HARDWARE.md](HARDWARE.md)
ตัวอย่าง BoardInfo ไม่ตั้งค่า GPIO ของอุปกรณ์ต่อพ่วง

```cpp
#include <RB_Nexus.h>
void setup() {
  Serial.begin(115200);
  Serial.println(RBNexus::name);
}
void loop() {}
```

## สร้างแพ็กเกจและเผยแพร่

ใช้ Python 3.9+ โดยไม่ต้องติดตั้ง Python library เพิ่ม:

```sh
python3 scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY
python3 scripts/verify_package.py
```

สคริปต์ดาวน์โหลด core เวอร์ชันที่ล็อกไว้ ตรวจ SHA-256 และสร้างไฟล์ใน dist/:
- RB_Nexus-esp32-0.1.1.zip สำหรับแนบ GitHub Release
- package_RB_Nexus_index.json สำหรับวางที่ root ของ branch main

ต้องนำ JSON และ ZIP จากการ build รอบเดียวกันมาใช้คู่กัน ห้ามแก้ checksum เอง
ZIP เป็นแบบไม่บีบอัดเพื่อให้ checksum เหมือนกันระหว่างเครื่องและ GitHub Actions

Workflow **Build and release** ทำงานเมื่อ push main หรือสั่ง Run workflow:
ตรวจ build → ติดตั้งแพ็กเกจผ่าน local HTTP ใน runner → compile ตัวอย่าง →
สร้าง Release v0.1.1 และแนบ ZIP/JSON → อัปเดต index ที่ root ของ main
หาก Release เวอร์ชันเดิมมีแล้ว workflow จะไม่เขียนทับไฟล์ ควรเพิ่ม VERSION
และเวอร์ชันไลบรารีก่อนเผยแพร่รอบใหม่

ถ้าขึ้น Connecting ค้าง ให้ตรวจสาย USB data และวงจร auto-reset;
ถ้าบอร์ดไม่มี auto-reset อาจต้องกด BOOT ระหว่างเริ่มอัปโหลด
หากไม่เห็น Port ให้ติดตั้งไดรเวอร์ที่ตรงกับชิป USB-UART จริงของบอร์ด

## เครดิต

พัฒนาบน [Arduino-ESP32 3.3.10](https://github.com/espressif/arduino-esp32/tree/3.3.10)
ของ Espressif Systems ดู [NOTICE.md](NOTICE.md) และ [LICENSE.md](LICENSE.md)

## Examples

เลือกบอร์ด **RB_Nexus** แล้วเปิด **File > Examples > RB_Nexus**
ทุกตัวอย่างใช้ Serial Monitor ที่ **115200 baud**

| Example | การใช้งาน |
| --- | --- |
| BoardInfo | แสดงชื่อบอร์ด ชิป Flash และ CPU |
| SerialEcho | พิมพ์ข้อความใน Serial Monitor แล้วรับข้อความเดิมกลับ |
| WiFiScan | สแกน Wi-Fi 2.4 GHz โดยไม่ต้องใส่รหัสผ่าน |
| DigitalInput | อ่าน HIGH/LOW จาก GPIO ที่กำหนด |
| AnalogInput | อ่าน ADC1 ภายใน ESP32 ความละเอียด 12-bit |
| PWMOutput | ปรับความสว่าง LED ภายนอกด้วย PWM 5 kHz |
| I2CScanner | ค้นหา address อุปกรณ์บนบัส I2C |
| CompileCheck | ตรวจการคอมไพล์แพ็กเกจสำหรับผู้พัฒนา |

ตัวอย่าง DigitalInput, AnalogInput, PWMOutput และ I2CScanner เริ่มต้นด้วยขา `-1`
เพื่อให้เปิดและคอมไพล์ได้ก่อน เมื่ออัปโหลดโดยยังไม่กำหนดขา จะแสดงข้อความทาง Serial
และยังไม่เริ่มใช้งาน GPIO ให้แก้ค่า `#define RB_EXAMPLE_..._PIN -1` ด้านบนของ sketch
เป็น GPIO ตามวงจรจริง ตรวจว่าขานั้นไม่ถูกใช้งานโดยวงจรอื่นอยู่แล้ว
เลขช่อง Analog/PWM ของคอนเน็กเตอร์ไม่ใช่หมายเลข GPIO โดยอัตโนมัติ

ยังไม่รวมตัวอย่างควบคุมมอเตอร์ เอ็นโค้ดเดอร์ หรือ Servo ของ RB_Nexus
เพราะต้องทราบผังขาและรุ่นชิปขับก่อน ไม่ควรนำ PWMOutput ไปแทนคำสั่งมอเตอร์โดยตรง

## อัปเดตจาก 0.1.0

ใช้ลิงก์ Boards Manager เดิม ค้นหา **RB_Nexus** แล้วเลือกอัปเดตเป็น **0.1.1**
ชื่อแพ็กเกจและชื่อบอร์ดแสดงเป็น **RB_Nexus** โดยไม่มีคำต่อท้าย

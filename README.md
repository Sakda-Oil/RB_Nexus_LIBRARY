# RB_Nexus

แพ็กเกจบอร์ดและไลบรารี RB_Nexus สำหรับ Arduino IDE / Boards Manager  
สำหรับบอร์ดหุ่นยนต์ **RB Nexus V0.1** (ESP32-WROOM-32 Dual Core 240MHz, 4MB Flash, USB-UART CH340, USB-C)  
พัฒนาบนสถาปัตยกรรม **Arduino-ESP32 Core 3.3.10 (ESP-IDF 5.1)** พร้อมระบบควบคุมหุ่นยนต์เต็มรูปแบบและรองรับ **ROS 2 Humble / Jazzy (micro-ROS)**

---

## เอกสารประกอบทางเทคนิค (Documentation & Interactive Portals)

- 📖 **[Interactive HTML Documentation](docs/html/index.html)** — เว็บไซต์คู่มือเชิงเทคนิคฉบับสมบูรณ์ (14 หน้า)
- 📌 **[Interactive Pinout Table](docs/pin_mapping.html)** — ตารางผังขาแบบฟิลเตอร์และค้นหาได้
- ✅ **[Hardware QC Test Checklist](docs/hardware_test_checklist.html)** — เช็กลิสต์ตรวจรับบอร์ดหน้างานสำหรับฝ่ายผลิตและ QC
- 📊 **[Validation Report (V0.1)](docs/VALIDATION_REPORT.md)** — รายงานการทดสอบทางวิศวกรรม ตอบครบทั้ง 5 ข้อกำหนดหลัก

---

## การติดตั้งใน Arduino IDE (Installation)

ใส่ลิงก์นี้ในช่อง **Additional Boards Manager URLs** ในการตั้งค่า (Settings/Preferences) ของ Arduino IDE:

```text
https://raw.githubusercontent.com/Sakda-Oil/RB_Nexus_LIBRARY/main/package_RB_Nexus_index.json
```

1. เปิด Arduino IDE (เวอร์ชัน 2.x ขึ้นไป)
2. ไปที่ **Preferences** (หรือ Settings)
3. วาง URL ด้านบนลงในช่อง **Additional Boards Manager URLs**
4. เปิดเมนู **Boards Manager** (ไอคอนบอร์ดทางซ้ายมือ)
5. พิมพ์ค้นหา `RB_Nexus` แล้วกด **Install** เวอร์ชันล่าสุด
6. ไปที่เมนู **Tools > Board > RB_Nexus > RB_Nexus**
7. เลือกพอร์ต Serial USB (**Tools > Port**)
8. เปิดตัวอย่าง **File > Examples > RB_Nexus > FullBoardTest** หรือ **RB_Nexus_SelfTest** แล้วกด Upload

---

## ผังขาและฮาร์ดแวร์ RB Nexus V0.1

| ส่วนการทำงาน | ช่อง / ขาที่เชื่อมต่อ | สถานะ Hardware จริง | รายละเอียด |
| :--- | :--- | :---: | :--- |
| **Status LED** | GPIO 2 | ✅ **PASS** | ไฟสถานะ LED บนบอร์ด (Active HIGH) |
| **Digital I/O** | GPIO 4, 12, 14, 26, 27 | ✅ **PASS** | ดิจิทัล I/O รองรับทั้ง `INPUT_PULLUP` และ `OUTPUT` |
| **Quadrature Encoders** | ENC1: GPIO 36, 39<br>ENC2: GPIO 34, 35<br>ENC3: GPIO 32, 33<br>ENC4: GPIO 25, 13 | ✅ **PASS** | นับพัลส์ 4 ล้ออิสระด้วย Hardware Interrupt ของ ESP32 |
| **Servo Outputs (8 ช่อง)** | PCA9685 I2C (0x40): CH8–CH15 | ✅ **PASS** | เอาต์พุตพัลส์มาตรฐาน 50 Hz สำหรับ RC Servos มุม 0–180° |
| **DC Motor 4 (M4)** | PCA9685 I2C (0x40): CH6, CH7 | ✅ **PASS** | ขับหมุนเดินหน้า ถอยหลัง เบรก และหยุดได้สมบูรณ์ |
| **DC Motor 2 (M2)** | PCA9685 I2C (0x40): CH2, CH3 | ⚠️ **PARTIAL** | หมุนได้ทิศทางเดียว (ลายวงจร CH3 ขาดบน V0.1) |
| **DC Motor 1, 3 (M1, M3)**| PCA9685 I2C (0x40): CH0,1 / CH4,5 | ❌ **FAIL** | มอเตอร์ไม่หมุน (ลายวงจรหรือเกต H-Bridge ไม่ตอบสนอง) |
| **MCP3208 12-bit ADC** | SPI (MOSI: 23, MISO: 19, SCK: 18, CS: 5) | ⚠️ **PENDING** | การสื่อสาร SPI ปกติ อยู่ระหว่างรอ Schematic เทียบสเกลแรงดัน |
| **I2C Bus** | SDA: GPIO 21, SCL: GPIO 22 | ✅ **PASS** | ตรวจพบชิป PCA9685 ที่แอดเดรส 0x40 |
| **CAN Bus (TWAI)** | RB_PIN_UNDEFINED (-1) | ⚠️ **UNVERIFIED** | ซอฟต์แวร์พร้อม รอชิป CAN Transceiver บน PCB |
| **IMU (6-DOF)** | I2C Auto-probe (0x68, 0x69) | ⚠️ **UNVERIFIED** | ซอฟต์แวร์พร้อม Auto-probe ป้องกันการแฮงก์ |

---

## ตัวอย่างการเรียกใช้งานโค้ด (`RB_Nexus.h`)

```cpp
#include <RB_Nexus.h>

void setup() {
  Nexus.begin(115200);   // เริ่มต้นระบบบอร์ด Serial, I2C, SPI, LED, PCA9685
  Nexus.encoderInit(1);  // เริ่มต้น Interrupt นับพัลส์เอนโค้ดเดอร์ช่อง 1

  Nexus.setLED(true);    // เปิดไฟ LED สถานะ

  // สั่งมอเตอร์ M4 หมุนเดินหน้า 80% (ความเร็ว -100 ถึง +100)
  Nexus.motorDrive(4, 80);

  // สั่งเซอร์โวช่อง 8 ไปที่มุม 90 องศา
  Nexus.servoWrite(8, 90);
}

void loop() {
  // อ่านค่าอนาล็อก 12-bit จาก MCP3208 ช่อง 0 (พอร์ต A1)
  uint16_t raw_adc = Nexus.analogReadMCP(0);
  float voltage = Nexus.analogReadVoltage(0);

  // อ่านค่าพัลส์เอนโค้ดเดอร์และคำนวณ RPM
  int32_t ticks = Nexus.getEncoderCount(1);
  float rpm = Nexus.getEncoderRPM(1, 330);

  Serial.printf("A1: %u (%.2fV) | ENC1: %ld (%0.1f RPM)\n", raw_adc, voltage, ticks, rpm);
  delay(100);
}
```

---

## แค็ตตาล็อกตัวอย่างโค้ด (Examples Directory)

ชุดตัวอย่างทั้งหมดกว่า 39 สเก็ตช์ ผ่านการทดสอบคอมไพล์ 100% บน `RB_Nexus:esp32:rb_nexus`:

### 1. ระบบฮาร์ดแวร์พื้นฐาน & Subsystem Tests
- **`RB_Nexus_SelfTest`**: สเก็ตช์ตรวจวัดตัวเองอัตโนมัติ (MCU, Flash, Wi-Fi, Bluetooth, PCA9685, MCP3208)
- **`FullBoardTest`**: เมนูทดสอบรวมทุกระบบแบบ Interactive สำหรับ QC
- **`MotorTest` & `MotorPID`**: ทดสอบมอเตอร์ M1..M4 พร้อมโหมด Diagnostic วิเคราะห์รายขา และระบบ Closed-loop PID
- **`ServoTest`**: กวาดมุมเซอร์โวมอเตอร์ 8 แชนแนล (CH8..CH15)
- **`EncoderTest`**: อ่านพัลส์เอนโค้ดเดอร์ 4 ชุดแบบเรียลไทม์
- **`AnalogMCP3208`**: อ่านค่า ADC 12-bit ทั้ง 8 ช่อง (A1..A8)
- **`DigitalIO` & `ButtonDebounce`**: ใช้งานพอร์ตดิจิทัล D4, D12, D14, D26, D27
- **`CANLoopback` & `IMURaw`**: สื่อสารผ่าน CAN Bus (TWAI) และเซนเซอร์วัดความเฉื่อย 6 แกน
- **`BluetoothSerialEcho` & `WiFiScan`**: การสื่อสารไร้สายของ ESP32

### 2. หุ่นยนต์ micro-ROS (ROS 2 Humble / Jazzy)
โฟลเดอร์ `examples/micro_ros/` บรรจุ 18 ตัวอย่างสำหรับการเชื่อมต่อกับ ROS 2:
- `01_Publisher` — Heartbeat String Publisher
- `02_Subscriber` / `03_LED_Subscriber` — รับคำสั่งควบคุมไฟ LED จาก ROS 2
- `04_Button_Publisher` — ส่งสถานะปุ่มกด/ดิจิทัลอินพุต
- `05_ADC_Publisher` / `06_MCP3208_AllChannels` — ส่งค่าอนาล็อก 12-bit
- `07_Encoder_Publisher` — ส่งค่าตำแหน่งล้อจากเอนโค้ดเดอร์
- `08_Motor_Subscriber` / `09_MotorEncoder` — รับคำสั่งความเร็วมอเตอร์พร้อมฟีดแบ็ก
- `10_IMU_Publisher` — ส่งข้อมูล `sensor_msgs/Imu`
- `11_Battery_Publisher` — ส่งระดับแรงดันแบตเตอรี่ `sensor_msgs/BatteryState`
- `12_DigitalIO` — ควบคุม I/O ผ่าน ROS 2
- `13_Servo_Subscriber` / `14_PWM_Subscriber` — ควบคุมเซอร์โวและ PWM
- `15_AllSensors` / `16_RobotStatus` — ส่งข้อมูลเซนเซอร์รวมและสถานะหุ่นยนต์
- `17_WiFiTransport` — micro-ROS UDP Transport ไร้สาย
- `18_SerialTransport` — micro-ROS Serial UART Transport ผ่าน USB

---

## สำหรับนักพัฒนาและวิศวกร (Developer Workflow)

### การสร้างแพ็กเกจ Boards Manager
```bash
python3 scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY
python3 scripts/verify_package.py
```

### การรันชุดทดสอบคอมไพล์ทุกตัวอย่าง
```bash
python3 scripts/compile_examples.py
```

---
*ลิขสิทธิ์ &copy; 2026 Redbrick Robotics*

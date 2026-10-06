> Historical report: describes an earlier revision. For 0.2.1 see [current test results](TEST_RESULTS_0.2.1.md) and [IMU guide](html/imu.html).

# RB_Nexus Hardware & Software Validation Report (V0.1 / Library v0.2.0)

**Date:** 2026-10-06
**Target Hardware:** RB_Nexus V0.1 (ESP32-WROOM-32, Dual-Core Xtensa LX6 @ 240MHz, 4MB Flash, USB-UART CH340)
**Package:** `RB_Nexus:esp32:rb_nexus` v0.2.0 (based on Arduino-ESP32 Core 3.3.10 / ESP-IDF 5.1)
**Repository:** [https://github.com/Sakda-Oil/RB_Nexus_LIBRARY](https://github.com/Sakda-Oil/RB_Nexus_LIBRARY)

---

## Executive Summary & The 5 Core Answers (สรุปคำตอบ 5 ข้อสำคัญ)

### 1. ส่วนไหน "ใช้งานจริงแล้ว" (Actually Verified on Real Hardware)
*ผลการตรวจสอบทางกายภาพและการทดสอบหน้างาน (QC Tested & Passed):*
- **Status LED:** GPIO 2 (Active HIGH) — ทดสอบไฟกะพริบและสถานะการทำงาน ผ่าน 100%
- **Digital I/O Ports:** D4, D12, D14, D26, D27 — ทดสอบทั้งสัญญาณขาเข้า `INPUT_PULLUP` และสัญญาณขาออก `OUTPUT` ผ่าน 100%
- **Quadrature Encoders (4 ชุด):**
  - Channel 1: Phase A (GPIO 36 / SENSOR_VP), Phase B (GPIO 39 / SENSOR_VN) — *Input-only pins, เหมาะสำหรับเอนโค้ดเดอร์* ผ่าน 100%
  - Channel 2: Phase A (GPIO 34), Phase B (GPIO 35) — *Input-only pins* ผ่าน 100%
  - Channel 3: Phase A (GPIO 32), Phase B (GPIO 33) ผ่าน 100%
  - Channel 4: Phase A (GPIO 25), Phase B (GPIO 13) ผ่าน 100%
- **Servo Motor Outputs (8 ช่อง):** ชิป PCA9685 ช่อง CH8 ถึง CH15 (I2C 0x40, SDA=21, SCL=22) ให้สัญญาณ PWM 50 Hz ขับ RC Servos ปรับมุม 0–180° ได้สมบูรณ์
- **DC Motor 4 (M4):** ขับผ่านชิป PCA9685 ช่อง CH6 (IN1) และ CH7 (IN2) หมุนเดินหน้า, ถอยหลัง, เบรก และหยุดได้สมบูรณ์

---

### 2. ส่วนไหน "Compile ผ่านแต่ยังไม่ได้ทดสอบ Hardware" (Compile Verified / Hardware Test Required)
*ระบบที่โค้ดไดรเวอร์เสร็จสมบูรณ์ คอมไพล์ผ่าน 100% ในชุดทดสอบ แต่ยังรอการทดสอบต่อพ่วงบนบอร์ดจริง:*
- **Closed-Loop PID Motor Speed Control:** คลาส `RB_Nexus_PID` คำนวณความเร็วมอเตอร์รอบปิดร่วมกับเอนโค้ดเดอร์ (คอมไพล์ผ่านทุกตัวอย่าง)
- **micro-ROS Architecture & 18 ROS 2 Examples:**
  - สถาปัตยกรรม State Machine (WAITING_AGENT, AGENT_AVAILABLE, CONNECTED, DISCONNECTED)
  - วอทช์ด็อกตัดกำลังมอเตอร์ฉุกเฉิน (E-Stop Watchdog)
  - ทั้ง 18 สเก็ตช์ตัวอย่าง (Publisher, Subscriber, cmd_vel, JointStates, IMU, Odom, Battery, Range, Mecanum, Services) คอมไพล์ผ่าน 100% บน `RB_Nexus:esp32:rb_nexus`
- **Wi-Fi & Bluetooth Stack:** ฟังก์ชันสแกน Wi-Fi และ Bluetooth Serial Echo (SerialBT) คอมไพล์ผ่าน
- **ESP32 LEDC Hardware Timers:** ไดรเวอร์ความถี่สูงสำหรับขา GPIO โดยตรง

---

### 3. ส่วนไหน "ยังขาดข้อมูลวงจร" (Lacks Circuit / Schematic Information)
*พินและระบบที่บอร์ด V0.1 มีหัวคอนเน็กเตอร์ แต่ยังไม่มีแผนผังวงจร (Schematic) หรือการต่อลายปริ้นต์ที่ชัดเจน:*
- **CAN Bus Subsystem (TWAI):**
  - ตัวบอร์ดมีพอร์ตต่อสาย CAN แต่ยังไม่มีข้อมูลว่าติดตั้งไอซี CAN Transceiver (เช่น SN65HVD230) ไว้บนบอร์ด หรือต้องต่อโมดูลภายนอก และยังไม่ได้ระบุ GPIO TX/RX บนลายปริ้นต์
  - *การจัดการในโค้ด:* กำหนดให้เป็น `RB_PIN_CAN_TX = RB_PIN_UNDEFINED (-1)` และ `RB_PIN_CAN_RX = RB_PIN_UNDEFINED (-1)` เพื่อป้องกันความเสียหายของพิน
- **Onboard IMU Subsystem:**
  - ไม่มีข้อมูลชิปไอซีที่แน่นอน (MPU6050, BMI160 หรือตัวอื่น) และไม่พบบน I2C Scanner เริ่มต้น
  - *การจัดการในโค้ด:* ไดรเวอร์ทำ Modular Auto-Probe หากพบไอซีที่แอดเดรส 0x68 หรือ 0x69 จะเริ่มทำงานอัตโนมัติ หากไม่พบจะข้ามอย่างปลอดภัย
- **MCP3208 Analog Channel Mapping:**
  - สัญญาณบัส SPI เชื่อมต่อได้ (MOSI=23, MISO=19, SCK=18, CS=5) แต่การจับคู่พอร์ต A1..A8 กับวงจรแบ่งแรงดัน (Voltage Divider) บนบอร์ดจริงยังต้องรอเอกสารวงจรชัดเจน

---

### 4. ส่วนไหน "ยังใช้งานไม่ได้" (Currently Non-Functional / Hardware Bug in V0.1)
*ข้อบกพร่องระดับฮาร์ดแวร์บน PCB รุ่น V0.1:*
- **DC Motor 2 (M2):** หมุนได้เพียงทิศทางเดียว (ลายวงจรสัญญาณ PCA9685 CH3 หรือ Gate ขาถอยหลังของ H-Bridge มีปัญหา)
- **DC Motor 1 (M1) และ DC Motor 3 (M3):** ไม่ทำงานเลย (ไม่มีแรงดันเอาต์พุตออกจากไดรเวอร์ ต้องตรวจสอบไฟเลี้ยงและวงจรขับ)

---

### 5. ต้องแก้อะไรตรงไหนต่อ สำหรับฮาร์ดแวร์รอบถัดไป (Modification Guide for Next Revision)

#### A. จุดที่ต้องแก้ไขบนแผ่นวงจรพิมพ์ (PCB Fixes for Hardware Revision 0.2):
1. **แก้ไขวงจรขับมอเตอร์ H-Bridge:**
   - ตรวจสอบลายปริ้นต์จาก PCA9685 CH0/CH1 (M1), CH2/CH3 (M2), CH4/CH5 (M3) ไปยังไดรเวอร์มอเตอร์
   - ซ่อมแซมขาสัญญาณถอยหลังของ M2 และวงจร Enable/Gate ของ M1 และ M3
2. **ติดตั้ง CAN Transceiver IC:**
   - ติดตั้งชิปแปลงระดับสัญญาณ CAN 3.3V (เช่น SN65HVD230) บนบอร์ด
   - เชื่อมต่อขา TX/RX ไปยัง GPIO ที่เหมาะสม (เช่น GPIO 4 และ GPIO 5)
3. **กำหนดวงจรและตำแหน่ง IMU:**
   - ติดตั้งเซนเซอร์ 6-DOF (เช่น MPU6050 หรือ ICM-42688) บนบัส I2C พิน SDA=21, SCL=22 และต่อขา INT เข้า GPIO สำหรับจับความเฉื่อย

#### B. จุดที่ต้องแก้ไขในซอฟต์แวร์ (Software & Library Modifications):
1. **การเปลี่ยนผังขา (Pinout Update):**
   - แก้ไขที่ไฟล์เดียว: `libraries/RB_Nexus/src/RB_Nexus_Pins.h` (Single Source of Truth)
   - หากเปลี่ยนพิน CAN ให้แก้ `#define RB_PIN_CAN_TX` และ `#define RB_PIN_CAN_RX`
2. **การสลับ Revision อัตโนมัติ (Revision Switching):**
   - ใน `RB_Nexus_Pins.h` ได้เตรียมโครงสร้าง Multi-Revision ไว้แล้ว:
   ```cpp
   #define RB_NEXUS_REV_01    1
   #define RB_NEXUS_REV_02    2
   #define RB_NEXUS_CURRENT_REV  RB_NEXUS_REV_02
   ```
3. **การ Rebuild แพ็กเกจ:**
   - ปรับเลขเวอร์ชันใน `VERSION` และ `libraries/RB_Nexus/library.properties`
   - รันคำสั่ง `python3 scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY`
   - รัน `python3 scripts/verify_package.py`
   - คัดลอก `dist/package_RB_Nexus_index.json` ไปยังรูทของ repository

---

## Comprehensive Subsystem Matrix (ตารางสรุปสถานะทุกระบบย่อย)

| ระบบย่อย (Subsystem) | พินฮาร์ดแวร์ / บัส | สถานะ Compile | สถานะ Hardware จริง | ข้อสังเกตเชิงลึกทางวิศวกรรม |
| :--- | :--- | :---: | :---: | :--- |
| **System LED** | GPIO 2 | ✅ PASS | ✅ PASS | ควบคุมผ่าน `Nexus.setLED()`, บูตสถานะปกติ |
| **Digital I/O** | D4, D12, D14, D26, D27 | ✅ PASS | ✅ PASS | รองรับ INPUT_PULLUP, OUTPUT ทุกพิน |
| **Encoders (ENC1..4)** | 36/39, 34/35, 32/33, 25/13 | ✅ PASS | ✅ PASS | ใช้ Hardware Interrupts นับพัลส์ 4 ล้ออิสระ |
| **PCA9685 I2C Core** | SDA=21, SCL=22 (0x40) | ✅ PASS | ✅ PASS | ตรวจพบที่แอดเดรส 0x40 ความถี่ PWM 50-1000Hz |
| **Servo 8–15** | PCA9685 CH8–15 | ✅ PASS | ✅ PASS | ให้พัลส์ 50 Hz (500–2500 µs) กวาด 0–180° |
| **Motor 4 (M4)** | PCA9685 CH6, CH7 | ✅ PASS | ✅ PASS | ไดรเวอร์ H-Bridge ทำงานสมบูรณ์ทั้งสองทิศ |
| **Motor 2 (M2)** | PCA9685 CH2, CH3 | ✅ PASS | ⚠️ PARTIAL | หมุนได้ทิศทางเดียว (ลายวงจร CH3 ขาด) |
| **Motor 1, 3 (M1, M3)**| PCA9685 CH0,1 / CH4,5 | ✅ PASS | ❌ FAIL | ไดรเวอร์ไม่ตอบสนอง ต้องแก้ PCB |
| **MCP3208 SPI ADC** | MOSI=23, MISO=19, SCK=18, CS=5 | ✅ PASS | ⚠️ PENDING | SPI ทำงานปกติ รอเทียบสเกล A1..A8 กับวงจรจริง |
| **CAN Bus (TWAI)** | RB_PIN_UNDEFINED (-1) | ✅ PASS | ⚠️ UNVERIFIED | โค้ดมีไดรเวอร์พร้อม รอไอซี Transceiver บน PCB |
| **IMU (6-DOF)** | Auto-probe I2C 0x68/0x69 | ✅ PASS | ⚠️ UNVERIFIED | ไดรเวอร์พร้อม ป้องกันแครชหากไม่พบไอซี |
| **PID Speed Control** | Software Math | ✅ PASS | ⚠️ READY | คลาส RB_Nexus_PID ทำงานในระดับเลขทศนิยม |
| **micro-ROS Core** | UART / UDP / Emulation | ✅ PASS | ⚠️ READY | State machine + Watchdog พร้อมรันกับ Agent |
| **18 micro-ROS Sketches** | ROS 2 Topics/Services | ✅ PASS | ⚠️ READY | คอมไพล์ผ่าน 100% ทุกตัวอย่าง |

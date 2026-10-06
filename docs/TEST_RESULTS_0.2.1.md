# การตรวจสอบ RB_Nexus 0.2.1 — 6 ตุลาคม 2026

## สภาพแวดล้อม

- ESP32-WROOM-32 target `RB_Nexus:esp32:rb_nexus`, Arduino-ESP32 3.3.10
- MPU9250 by hideakitai 0.4.8
- Adafruit BNO08x 1.2.7, BusIO 1.17.4, Unified Sensor 1.1.15
- micro_ros_arduino 2.0.8-jazzy, commit `5917738c089d353c47885a0f0926fa0de1ee6cf2`
- Windows, Arduino CLI 1.5.2-rc.1; host regression tests ใช้ Zig C++ 0.13.0

## ผลที่ยืนยันในเครื่อง

- PASS: compile/link ไดรเวอร์ IMU ด้วยไลบรารี MPU9250 และ BNO08x จริง
- PASS: compile/link ตัวอย่าง 15_AllSensors ด้วย micro-ROS จริงบน core 3.3.10
- PASS: host regression 11 กลุ่ม ใช้ production driver กับ fake I2C/clock/vendor events
  - หยุด PID แล้วไม่กลับมาหมุน, manual override, zero-RPM stop
  - clamp ความเร็วมอเตอร์และ emergency latch ร่วมระหว่าง aliases
  - command timeout 500 ms รวมกรณี millis rollover
  - dead-time หลังหยุด/กลับทิศและ software watchdog
  - servo pulse ตามความถี่และ GPIO PWM ซ้ำหลายครั้ง
  - CAN flags และ validation
  - MPU6050 identity/ranges, signed SI conversion และ failed-read handling
  - MPU9250 identity, SI units และ stale sample
  - BNO085 report freshness แยก accel/gyro และ quaternion normalization
  - BNO085 reset recovery, enable-report failure และ address validation

## ผล GitHub Actions ที่ยืนยันแล้ว

ตรวจผลสำเร็จวันที่ 7 ตุลาคม 2026 สำหรับซอร์ส commit `2467fbf59c4f7632379f10f35699445db342be61`:

- PASS: compile/link **44/44 builds** — ตัวอย่าง 39 sketches และ GPIO overrides 5 กรณี ไม่มี build ล้มเหลว
- PASS: ตัวอย่าง micro-ROS ทั้ง 18 ไฟล์ใช้ไลบรารีจริง ไม่ใช้ fallback
- PASS: BoardInfo ด้วย `FlashSize=8M,PartitionScheme=default_8MB`
- PASS: host regressions, local HTML links, package checksum/layout/source และการติดตั้งแพ็กเกจ
- SUCCESS: เผยแพร่ Release `v0.2.1` พร้อม archive และ Boards Manager index
- PASS: วันที่ 7 ตุลาคม 2026 ใช้ Arduino CLI ดาวน์โหลดและติดตั้ง `RB_Nexus:esp32@0.2.1` จาก public Boards Manager URL สำเร็จ (ตรวจ checksum โดยตัวติดตั้ง); แก้ปัญหาลิงก์ไฟล์ติดตั้ง 404 ของรุ่นเดิมแล้ว

หลักฐาน: [CI Verification](https://github.com/Sakda-Oil/RB_Nexus_LIBRARY/actions/runs/37480356808), [Build and release](https://github.com/Sakda-Oil/RB_Nexus_LIBRARY/actions/runs/37480356723), [Release 0.2.1](https://github.com/Sakda-Oil/RB_Nexus_LIBRARY/releases/tag/v0.2.1)

ผลเหล่านี้เป็นการตรวจซอฟต์แวร์ ไม่ใช่การอ้างว่าฮาร์ดแวร์ผ่าน QC แล้ว

## การตรวจเอกสาร

ตรวจหน้า IMU ในเบราว์เซอร์ ปรับการตัดบรรทัดบนจอแคบ และตรวจ local links/UTF-8 ด้วย scripts/check_docs.py ตัวอย่างเต็มใน README ตรงกับ IMURaw ที่นำไป compile

## ยังต้องตรวจบนบอร์ดจริง

- การสื่อสาร I2C กับ MPU6050, MPU9250 และ GY-BNO085 รุ่นโมดูลที่ใช้จริง
- Bias, scale, ทิศของแกน, calibration และ orientation เมื่อเคลื่อนที่
- การเชื่อมต่อ ROS Agent, timestamp sync, latency และการถอดสายขณะมอเตอร์หมุน
- ข้อจำกัด PCB M1/M2/M3 จากรายงานเดิม

ผล host tests เป็นการตรวจ logic ไม่ใช่การจำลองไฟฟ้า ส่วน compile/link ไม่ยืนยัน runtime บนบอร์ด

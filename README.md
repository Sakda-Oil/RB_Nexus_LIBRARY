# RB_Nexus 0.2.2

Arduino board package และไลบรารีสำหรับ RB Nexus V0.1 (ESP32-WROOM-32) บน Arduino-ESP32 core 3.3.10

รองรับ PCA9685 มอเตอร์/Servo, MCP3208, Encoder, **MPU6050, MPU6500, MPU9250 + AK8963 และ GY-BNO085 ผ่าน I2C** พร้อมตัวอย่าง micro-ROS ที่ใช้ไลบรารีจริง

## ติดตั้ง

เพิ่ม URL ใน Arduino IDE → Preferences → Additional Boards Manager URLs:

```text
https://raw.githubusercontent.com/Sakda-Oil/RB_Nexus_LIBRARY/main/package_RB_Nexus_index.json
```

ติดตั้งบอร์ด RB_Nexus จาก Boards Manager รุ่น 0.2.2 ขึ้นไปมีรายการไลบรารีที่ต้องใช้ในตัว: **MPU9250 by hideakitai 0.4.8**, **Adafruit BNO08x 1.2.7**, **Adafruit BusIO 1.17.4**, **Adafruit Unified Sensor 1.1.15**

Arduino IDE 2.4.0 / Arduino CLI 1.5.0 ขึ้นไปรองรับการติดตั้งไลบรารีเหล่านี้พร้อมบอร์ด หากมีรุ่นเท่ากับหรือใหม่กว่าที่กำหนดแล้วจะเก็บรุ่นเดิมไว้ ตาม [ข้อกำหนด Arduino](https://docs.arduino.cc/arduino-cli/package_index_json-specification/#platforms-definitions) สำหรับ IDE รุ่นเก่าให้ติดตั้งผ่าน Library Manager และเลือก Install All; ต้องมีทั้ง MPU9250 และ BNO08x แม้ใช้ MPU6500 ตัวเดียว หากไฟล์หลักขาด โค้ดจะแจ้งชื่อไลบรารีที่ต้องติดตั้งตอน Verify/Upload

ตรวจไลบรารีและไฟล์หลักโดยไม่ติดตั้งเพิ่มด้วย `python scripts/install_dependencies.py --check-only --skip-microros` ถ้าขาดหรือรุ่นเก่า โปรแกรมจะแจ้งรายการและจบด้วยรหัส 1 ตัด `--check-only` ออกเพื่อติดตั้งรุ่นที่ใช้ทดสอบ ต้องติดตั้ง Arduino CLI และใช้ `--config-file` ให้ตรงกับโฟลเดอร์ Sketchbook ของ IDE; ผลตรวจของโปรไฟล์ทดสอบไม่ได้ยืนยันโปรไฟล์ IDE อีกชุดหนึ่ง

ตัวอย่าง ROS ต้องใช้ micro_ros_arduino เพิ่มด้วย: เรียกตัวติดตั้งโดยไม่ใส่ `--skip-microros` เพื่อใช้ Jazzy snapshot ที่โครงการทดสอบ ไม่รวม micro-ROS ในรายการติดตั้งอัตโนมัติของบอร์ด เพราะต้องใช้ snapshot เฉพาะ

## อ่าน IMU

SDA=GPIO21, SCL=GPIO22, GND ร่วม; สัญญาณ I2C 3.3 V ใช้ไฟเลี้ยงตามสเปกโมดูลจริง

```cpp
#include <RB_Nexus.h>

void setup() {
  Serial.begin(115200);
  RB.begin();
  // เลือก Auto, MPU6050, MPU6500, MPU9250 หรือ BNO085 (ครั้งละหนึ่งตัว)
  if (!RB.imuBegin(RBIMUType::Auto)) Serial.println("IMU initialization failed; check wiring and WHO_AM_I");
  else Serial.println(RB.imuModelName());
}
void loop() {
  RB.update();
  static uint32_t last = 0;
  if (millis() - last >= 100) {
    last = millis();
    if (RB.imuDataFresh()) {
      Serial.printf("a[m/s^2] %.3f %.3f %.3f  gyro[rad/s] %.3f %.3f %.3f\n",
        RB.accelX(), RB.accelY(), RB.accelZ(), RB.gyroX(), RB.gyroY(), RB.gyroZ());
    } else Serial.println("Waiting for fresh IMU data");
  }
}
```

Auto เลือก BNO085 ก่อน MPU9250/MPU9255 แล้วจึง MPU6050/MPU6500; ใช้หนึ่งเซนเซอร์ในแต่ละครั้ง เลือกชนิด/address เองได้ผ่าน imuBegin(type, address) ค่า WHO_AM_I=0x70 จะเลือกไดรเวอร์ MPU6500 และรายงานชื่อจริง แม้โมดูลจะติดป้าย MPU9250; การตอบ ACK หรือ WHO_AM_I ไม่ใช่หลักฐานยืนยันของแท้
ค่าความเร่ง m/s² รวม gravity, gyro rad/s, magnetic field µT และ Euler องศา ต้องตรวจ imuDataFresh() ก่อนใช้ข้อมูล

MPU6050/MPU6500 รองรับ accel/gyro 6 แกน ไม่มี magnetometer และยังไม่ทำ orientation fusion: Euler/quaternion เป็น NaN และ imuOrientationFresh() เป็น false

## วัดองศาหมุนรอบแกน Z

เปิด File → Examples → RB_Nexus → **IMURotationZ** อัปโหลดแล้วเปิด Serial Monitor 115200 วางเซนเซอร์นิ่ง 3 วินาทีเพื่อหาค่าชดเชย จากนั้นหมุนรอบแกน Z ของเซนเซอร์ จะเห็นมุมสะสม เช่น 90°, 180°, 360° และส่งตัวเลข `0` เพื่อตั้งศูนย์ใหม่

```cpp
RB.update();
if (RB.imuRotationZFresh()) {
  Serial.println(RB.imuRotationZ()); // องศาสะสม มีเครื่องหมาย ไม่จำกัด 360
}
// RB.imuResetRotationZ();          // ตั้งจุดอ้างอิงใหม่ รอ gyro ตัวอย่างถัดไป
// RB.imuSetGyroZBias(biasRadS);     // ค่าเฉลี่ย gyroZ() ขณะวางนิ่ง หน่วย rad/s
```

ค่าบวกหมุนตามกฎมือขวารอบแกน Z; ติดตั้งแกน Z ตั้งฉากพื้นเพื่อวัดการเลี้ยวหุ่นยนต์ ค่านี้คำนวณจาก gyro จึงสะสมความคลาดเคลื่อนและไม่ใช่มุมเทียบทิศเหนือ ใช้ได้กับ IMU ทุกชนิดที่รองรับ เรียก update ต่อเนื่องทุก 5–20 ms; ก่อนมีข้อมูลจะได้ NaN และช่วงข้อมูลหายเกิน 250 ms จะไม่ประมาณมุมที่ขาดหาย ต้องตั้งศูนย์ใหม่หากเคลื่อนที่ระหว่างช่วงนั้น ความสดของข้อมูลไม่ได้รับรองความแม่นยำของมุม สำหรับ MPU9250/BNO085 ยังอ่านมุมจาก sensor fusion ได้ทาง yaw() โดยตรวจ imuOrientationFresh()

## ควบคุมมอเตอร์

```cpp
RB.motorSet(4, 128);  // ช่วง -255..255
RB.motorStop(4);      // หยุดและปิด PID
RB.emergencyStop();   // latch จนเรียก clearEmergencyStop()
```

เรียก RB.update() ใน loop สำหรับ PID, IMU และ software watchdog. ตัวอย่าง ROS มอเตอร์หยุดเมื่อไม่มีคำสั่ง 500 ms; ส่งคำสั่งซ้ำอย่างน้อย 10 Hz

## CAN Bus

ขาตามวงจร: **TX=GPIO17, RX=GPIO16** เริ่มใช้งานด้วย `RB.canBegin(500000)` สำหรับบัส 500 kbit/s หรือระบุขาของ transceiver ภายนอกด้วย `RB.canBegin(500000, txPin, rxPin)` ตัวอย่าง CANLoopback ใช้ normal mode ต้องมี CAN node อีกตัวรับและ ACK พร้อม termination ที่ปลายบัส

กำหนดค่าเริ่มต้น CAN ให้ตรงวงจร (TX=GPIO17, RX=GPIO16) และให้ SelfTest แสดงว่ากำหนดขาแล้วโดยไม่อ้างว่าฮาร์ดแวร์ผ่านการทดสอบ

## การทดสอบ

```sh
python scripts/install_dependencies.py --config-file arduino-cli.yaml
python scripts/test_host.py
python -m unittest discover -s tests -p "test_*.py"
python scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY
python scripts/verify_package.py
python scripts/compile_examples.py --config-file arduino-cli.yaml
```

ยังต้องทดสอบบนบอร์ดจริง โดยเฉพาะ I2C ของ BNO085, calibration/แกน IMU และการหยุดมอเตอร์ รายงานเดิมระบุข้อจำกัด PCB ของ M1/M2/M3; การแก้ซอฟต์แวร์ไม่ได้ยืนยันว่าแก้ปัญหาวงจรแล้ว

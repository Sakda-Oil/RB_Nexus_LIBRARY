# RB_Nexus 0.2.1

Arduino board package และไลบรารีสำหรับ RB Nexus V0.1 (ESP32-WROOM-32) บน Arduino-ESP32 core 3.3.10

รองรับ PCA9685 มอเตอร์/Servo, MCP3208, Encoder, **MPU6050, MPU9250 + AK8963 และ GY-BNO085 ผ่าน I2C** พร้อมตัวอย่าง micro-ROS ที่ใช้ไลบรารีจริง

## ติดตั้ง

เพิ่ม URL ใน Arduino IDE → Preferences → Additional Boards Manager URLs:

```text
https://raw.githubusercontent.com/Sakda-Oil/RB_Nexus_LIBRARY/main/package_RB_Nexus_index.json
```

ติดตั้งบอร์ด RB_Nexus จาก Boards Manager แล้วติดตั้ง **MPU9250 by hideakitai 0.4.8** และ **Adafruit BNO08x 1.2.7** พร้อม dependencies จาก Library Manager (ต้องติดตั้งทั้งคู่)

## อ่าน IMU

SDA=GPIO21, SCL=GPIO22, GND ร่วม; สัญญาณ I2C 3.3 V ใช้ไฟเลี้ยงตามสเปกโมดูลจริง

```cpp
#include <RB_Nexus.h>

void setup() {
  Serial.begin(115200);
  RB.begin();
  // เลือก Auto, MPU6050, MPU9250 หรือ BNO085 (หนึ่งตัวทำงานในแต่ละครั้ง)
  if (!RB.imuBegin(RBIMUType::Auto)) Serial.println("IMU not found");
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

Auto เลือก BNO085 ก่อน MPU9250 แล้วจึง MPU6050; ใช้หนึ่งเซนเซอร์ในแต่ละครั้ง เลือกชนิด/address เองได้ผ่าน imuBegin(type, address)
ค่าความเร่ง m/s² รวม gravity, gyro rad/s, magnetic field µT และ Euler องศา ต้องตรวจ imuDataFresh() ก่อนใช้ข้อมูล

MPU6050 รองรับ accel/gyro 6 แกน ไม่มี magnetometer และยังไม่ทำ orientation fusion: Euler/quaternion เป็น NaN และ imuOrientationFresh() เป็น false

## ควบคุมมอเตอร์

```cpp
RB.motorSet(4, 128);  // ช่วง -255..255
RB.motorStop(4);      // หยุดและปิด PID
RB.emergencyStop();   // latch จนเรียก clearEmergencyStop()
```

เรียก RB.update() ใน loop สำหรับ PID, IMU และ software watchdog. ตัวอย่าง ROS มอเตอร์หยุดเมื่อไม่มีคำสั่ง 500 ms; ส่งคำสั่งซ้ำอย่างน้อย 10 Hz

## คู่มือและการทดสอบ

- [คู่มือ HTML](docs/html/index.html)
- [MPU6050 / MPU9250 / GY-BNO085](docs/html/imu.html)
- [API](docs/html/api.html)
- [micro-ROS](docs/html/microros.html)
- [สอนแก้โค้ดด้วยตัวเอง](docs/html/how_to_modify.html)
- [เปลี่ยนเวอร์ชันและเผยแพร่รุ่นใหม่](docs/html/release.html)
- [เช็กลิสต์ดูแลและอัปเดตฉบับย่อ](docs/MAINTENANCE_QUICKSTART_TH.md)
- [ผลการตรวจรุ่น 0.2.1](docs/TEST_RESULTS_0.2.1.md)
- [รายงานข้อผิดพลาดก่อนแก้](docs/CODE_REVIEW_2026-10-06.md)

```sh
python scripts/install_dependencies.py --config-file arduino-cli.yaml
python scripts/test_host.py
python scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY
python scripts/verify_package.py
python scripts/compile_examples.py --config-file arduino-cli.yaml
```

ยังต้องทดสอบบนบอร์ดจริง โดยเฉพาะ I2C ของ BNO085, calibration/แกน IMU และการหยุดมอเตอร์ รายงานเดิมระบุข้อจำกัด PCB ของ M1/M2/M3; การแก้ซอฟต์แวร์ไม่ได้ยืนยันว่าแก้ปัญหาวงจรแล้ว

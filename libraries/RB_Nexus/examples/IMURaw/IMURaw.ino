#include <RB_Nexus.h>

// ตัวอย่างการตรวจหาและอ่านค่าเซนเซอร์ความเฉื่อย (IMU)
// หมายเหตุทางวิศวกรรม:
//   บอร์ด V0.1 มีคอนเน็กเตอร์ IMU เชื่อมต่อบัส I2C แต่ยังไม่ระบุเบอร์ชิปและ Address
//   ไลบรารีจะทำการ Auto-probe ค้นหาชิปตระกูล MPU6050, LSM6DS3, BMI270 อัตโนมัติ

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - IMU Auto-Probe & Diagnostics");
  Serial.println("Status: UNVERIFIED ON V0.1 PCB (Pending Schematic)");
  Serial.println("=========================================");

  RB.begin();

  Serial.println("Scanning I2C for IMU sensor...");
  bool found = RB.imuBegin();

  if (found) {
    Serial.printf("IMU Sensor Detected: %s\n", RB.imuModelName());
  } else {
    Serial.println("No supported IMU detected on I2C bus.");
    Serial.println("Please check if an IMU module is installed on the connector.");
  }
}

void loop() {
  RB.update();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    lastPrint = millis();

    if (RB.isIMUAvailable()) {
      Serial.printf("Accel (g): X=%.2f Y=%.2f Z=%.2f | Gyro (deg/s): X=%.2f Y=%.2f Z=%.2f\n",
                    RB.accelX(), RB.accelY(), RB.accelZ(),
                    RB.gyroX(), RB.gyroY(), RB.gyroZ());
      RB.toggleLED();
    } else {
      Serial.println("IMU not connected. Waiting for hardware module...");
    }
  }
}

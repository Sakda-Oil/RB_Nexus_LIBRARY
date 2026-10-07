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

#include <RB_Nexus.h>

void setup() {
  Serial.begin(115200);
  Serial.printf("RB_Nexus %s - BNO085 diagnostic\n", RBNexus::version);
  RB.begin(); // SDA=21, SCL=22; shared 3.3 V I2C logic
  RB.i2cScan();
  // Explicit selection prevents an MPU from hiding a failed BNO085 connection.
  if (!RB.imuBegin(RBIMUType::BNO085)) {
    Serial.println("BNO085 init failed. Check 0x4A/0x4B, power, GND and module I2C mode.");
    return;
  }
  Serial.println("BNO085 connected. Send 0 to zero Z. With +Z up: right +, left -.");
  Serial.println("Rotate flat by about 90 degrees. Z is accumulated degrees; gyro is rad/s; yaw is fused heading.");
}

void loop() {
  RB.update();
  if (Serial.available() && Serial.read() == '0') RB.imuResetRotationZ();
  static uint32_t last = 0, previousSamples = 0;
  if (millis() - last >= 1000) {
    last = millis();
    const uint32_t samples = RB.imuGyroSampleCount();
    Serial.printf("gyro samples=%lu (+%lu) raw=%.4f rad/s Z=%.2f deg dataFresh=%d rotationFresh=%d orientationFresh=%d\n",
      (unsigned long)samples, (unsigned long)(samples-previousSamples), RB.gyroZ(), RB.imuRotationZ(),
      RB.imuDataFresh(), RB.imuRotationZFresh(), RB.imuOrientationFresh());
    previousSamples = samples;
    Serial.printf("gyro XYZ=%.4f, %.4f, %.4f rad/s; fused yaw=%.2f deg\n",
      RB.gyroX(), RB.gyroY(), RB.gyroZ(), RB.yaw());
    if (!RB.isIMUAvailable()) Serial.println("BNO085 unavailable; check startup messages and connection.");
    else if (!RB.imuRotationZFresh()) Serial.println("No fresh gyro reports; initialized does not mean data is arriving.");
  }
}

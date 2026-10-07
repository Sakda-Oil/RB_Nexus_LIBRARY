#include <RB_Nexus.h>
#include <math.h>

bool ready = false;

void setup() {
  Serial.begin(115200);
  RB.begin();
  if (!RB.imuBegin(RBIMUType::Auto)) {
    Serial.println("IMU initialization failed. Check wiring, power and WHO_AM_I.");
    RB.i2cScan();
    return;
  }
  Serial.println(RB.imuModelName());
  if (RB.imuType() == RBIMUType::BNO085) {
    Serial.println("BNO085: internal sensor fusion active (factory/runtime calibrated).");
    RB.imuSetGyroZBias(0.0f);
  } else {
    Serial.println("Keep the sensor STILL for gyro bias calibration (3 seconds)...");
    delay(500);
    float sum = 0;
    unsigned count = 0;
    const uint32_t start = millis();
    while (millis() - start < 3000 && count < 100) {
      if (RB.imuUpdate() && isfinite(RB.gyroZ())) {
        sum += RB.gyroZ();
        ++count;
      }
      delay(10);
    }
    if (count > 0) {
      RB.imuSetGyroZBias(sum / count); // rad/s, measured while stationary only
      Serial.printf("Gyro bias calibrated: %.4f rad/s (%u samples)\n", sum / count, count);
    }
  }
  RB.imuResetRotationZ();
  ready = true;
  Serial.println("Relative Z rotation in degrees: Turn Right (+), Turn Left (-). Send '0' to zero.");
  Serial.println("Keep the Z axis vertical for robot turn angle. Drift is possible on 6-axis sensors.");
}

void loop() {
  RB.update();
  if (!ready) return;
  if (Serial.available() && Serial.read() == '0') RB.imuResetRotationZ();
  static uint32_t last = 0;
  if (millis() - last >= 100) {
    last = millis();
    if (RB.imuRotationZFresh()) {
      Serial.printf("Z rotation: %.2f deg\n", RB.imuRotationZ());
    } else Serial.println("Waiting for fresh gyro data");
  }
}

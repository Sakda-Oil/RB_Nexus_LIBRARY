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
  Serial.println("Keep the sensor STILL for gyro bias calibration (3 seconds).");
  delay(1000);
  float sum = 0;
  unsigned count = 0;
  const uint32_t start = millis();
  while (millis() - start < 2000) {
    RB.imuUpdate();
    // Short freshness window also works with BNO085's independent reports.
    if (RB.imuRotationZFresh(10) && isfinite(RB.gyroZ())) {
      sum += RB.gyroZ();
      ++count;
    }
    delay(20);
  }
  if (count < 50) {
    Serial.println("Not enough gyro samples. Check the sensor and restart.");
    return;
  }
  RB.imuSetGyroZBias(sum / count); // rad/s, measured while stationary only
  RB.imuResetRotationZ();
  ready = true;
  Serial.println("Relative Z rotation in degrees; send 0 to zero. Drift is possible.");
  Serial.println("Keep the Z axis vertical for a robot's turn angle. This is not north heading.");
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

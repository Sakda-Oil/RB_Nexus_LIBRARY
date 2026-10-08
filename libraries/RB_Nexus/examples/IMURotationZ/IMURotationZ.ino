#include <RB_Nexus.h>
#include <math.h>

bool ready = false;

void setup() {
  Serial.begin(115200);
  Serial.printf("RB_Nexus %s - Z rotation\n", RBNexus::version);
  RB.begin();
  if (!RB.imuBegin(RBIMUType::Auto)) {
    Serial.println("IMU initialization failed. Check wiring, power and WHO_AM_I.");
    RB.i2cScan();
    return;
  }
  Serial.println(RB.imuModelName());
  if (RB.imuType() == RBIMUType::BNO085) {
    Serial.println("BNO085: using internally calibrated gyro; no extra manual gyro bias.");
    RB.imuSetGyroZBias(0.0f);
    const uint32_t start = millis();
    while (!RB.imuRotationZFresh() && millis() - start < 3000) {
      RB.imuUpdate();
      delay(5);
    }
    if (!RB.imuRotationZFresh()) {
      Serial.println("BNO085 initialized, but no gyro reports. Run BNO085Diagnostic.");
      return;
    }
  } else {
    Serial.println("Keep the sensor STILL for gyro bias calibration (3 seconds)...");
    const uint32_t warmup = millis();
    while (millis() - warmup < 500) { RB.imuUpdate(); delay(5); }
    float sum = 0;
    unsigned count = 0;
    uint32_t previousSample = RB.imuGyroSampleCount();
    const uint32_t start = millis();
    while (millis() - start < 2500) {
      RB.imuUpdate();
      const uint32_t sample = RB.imuGyroSampleCount();
      if (sample != previousSample && isfinite(RB.gyroZ())) {
        previousSample = sample;
        sum += RB.gyroZ();
        ++count;
      }
      delay(5);
    }
    if (count >= 50) {
      RB.imuSetGyroZBias(sum / count); // rad/s, measured while stationary only
      Serial.printf("Gyro bias calibrated: %.4f rad/s (%u samples)\n", sum / count, count);
    } else { Serial.println("Not enough new gyro samples. Check sensor and restart."); return; }
  }
  RB.zeroZ();
  ready = true;
  Serial.println("Relative Z rotation in degrees: Turn Right (+), Turn Left (-). Send '0' to zero.");
  Serial.println("Keep sensor +Z pointing UP. This is gyro integration; drift is possible on all sensors.");
}

void loop() {
  RB.update();
  if (!ready) return;
  if (Serial.available() && Serial.read() == '0') RB.zeroZ();
  static uint32_t last = 0;
  if (millis() - last >= 100) {
    last = millis();
    if (RB.imuRotationZFresh()) {
      Serial.printf("Z rotation: %.2f deg\n", RB.angleZ());
    } else Serial.println("Waiting for fresh gyro data");
  }
}

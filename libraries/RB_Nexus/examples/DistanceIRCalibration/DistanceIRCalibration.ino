#include <RB_Nexus.h>
#include <math.h>

// Put a flat target at known distances and record Voltage using Serial Monitor.
// Replace EXAMPLE points with your measurements (cm increases, volts decreases).
RBIRDistanceSensor frontDistance(1, RBIRModel::Custom);
const RBIRCalibrationPoint measuredPoints[] = {{10,2.30f},{20,1.30f},{40,0.72f},{80,0.40f}};

void setup() {
  Serial.begin(115200);
  RB.begin();
  if (!frontDistance.begin()) Serial.println("Use analog channel 1..8");
  if (!frontDistance.setCalibration(measuredPoints, 4)) Serial.println("Invalid calibration table");
}
void loop() {
  RB.update();
  static uint32_t lastRead = 0;
  if (millis() - lastRead >= 200) {
    lastRead = millis();
    Serial.printf("Voltage: %.3f V\n", frontDistance.volts());
    float cm = frontDistance.cm();
    if (isfinite(cm)) Serial.printf("Distance: %.1f cm\n", cm);
    else Serial.println(frontDistance.statusText());
  }
}

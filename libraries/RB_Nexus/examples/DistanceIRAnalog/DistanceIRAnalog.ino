#include <RB_Nexus.h>
#include <math.h>

// Sharp GP2Y0A21YK0F (10..80cm): supply 5V, common GND, OUT -> A1.
// A1 connector supplies 3.3V: power this sensor separately from a suitable 5V rail.
// Keep the ADC signal <=3.3V. Use a ruler to calibrate for your actual sensor.
RBIRDistanceSensor frontDistance(1, RBIRModel::Sharp10To80CM);

void setup() {
  Serial.begin(115200);
  RB.begin();
  if (!frontDistance.begin()) Serial.println("Use analog channel 1..8");
}
void loop() {
  RB.update();
  static uint32_t lastRead = 0;
  if (millis() - lastRead >= 100) {
    lastRead = millis();
    float cm = frontDistance.cm();
    if (isfinite(cm)) Serial.printf("Approximate IR distance: %.1f cm\n", cm);
    else Serial.println(frontDistance.statusText());
    // Below 10cm the Sharp curve folds back: it may falsely appear farther away.
  }
}

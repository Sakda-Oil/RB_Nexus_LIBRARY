#include <RB_Nexus.h>
#include <math.h>

// Project: turn LED on within 20cm; blink LED when distance is unknown.
// HC-SR04: TRIG=D26, ECHO=D27 through a 5V-to-3.3V divider, VCC=5V, shared GND.
RBUltrasonicSensor frontDistance(RB_PIN_D26, RB_PIN_D27);
const float warningDistanceCm = 20.0f;

void setup() {
  Serial.begin(115200);
  RB.begin();
  if (!frontDistance.begin()) Serial.println("Invalid sensor pins");
}
void loop() {
  RB.update();
  static uint32_t lastRead = 0;
  if (millis() - lastRead >= 100) {
    lastRead = millis();
    float cm = frontDistance.cm();
    if (!isfinite(cm)) {
      RB.led((millis() / 300) % 2);
      Serial.println(frontDistance.statusText());
      return; // Unknown distance must not be treated as a clear path.
    }
    RB.led(cm <= warningDistanceCm);
    Serial.printf("%.1f cm: %s\n", cm, cm <= warningDistanceCm ? "NEAR" : "FAR");
  }
}

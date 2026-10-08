#include <RB_Nexus.h>
#include <math.h>

// HC-SR04: VCC=5V, common GND, TRIG=D26.
// ECHO -> 2.2k resistor -> D27; D27 -> 3.3k resistor -> GND (about 3V).
// NEVER connect the 5V ECHO signal directly to ESP32.
RBUltrasonicSensor frontDistance(RB_PIN_D26, RB_PIN_D27);

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
    if (isfinite(cm)) Serial.printf("Distance: %.1f cm\n", cm);
    else Serial.println(frontDistance.statusText());
  }
}

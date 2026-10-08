#include <RB_Nexus.h>

// IR obstacle module: digital OUT -> D4, GND shared, signal <=3.3V.
// Check the module's supply voltage and output circuit before connecting.
// Most modules detect with LOW; pass false as second argument for active HIGH.
RBIRObstacleSensor frontObstacle(RB_PIN_D4);

void setup() {
  Serial.begin(115200);
  RB.begin();
  if (!frontObstacle.begin()) Serial.println("Invalid sensor pin");
}
void loop() {
  RB.update();
  static uint32_t lastRead = 0;
  if (millis() - lastRead >= 100) {
    lastRead = millis();
    if (!frontObstacle.isReady()) return;
    bool detected = frontObstacle.detected();
    RB.led(detected);
    Serial.println(detected ? "Obstacle detected" : "No obstacle detected");
    // This sensor provides a yes/no signal, NOT distance in centimeters.
  }
}

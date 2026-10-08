#include <RB_Nexus.h>

// M4 example: no movement at startup. Lift wheels before testing.
// Send f=forward 30%, b=backward 30%, s=stop, e=emergency stop, r=unlock.
// Each movement lasts at most one second; repeat f/b to keep moving.
const uint8_t motorChannel = 4;
bool boardReady = false, moving = false;
uint32_t commandTime = 0;

void setup() {
  Serial.begin(115200);
  boardReady = RB.begin();
  RB.stopAll();
  Serial.println(boardReady ? "f forward, b backward, s stop, e emergency, r unlock" : "PCA9685 missing: check board");
}
void loop() {
  RB.update();
  if (!boardReady) return;
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 's') { RB.stop(motorChannel); moving = false; }
    if (command == 'e') { RB.emergencyStop(); moving = false; }
    if (command == 'r') RB.clearEmergencyStop();
    if ((command == 'f' || command == 'b') && !RB.isEmergencyStopped()) {
      RB.motorPercent(motorChannel, command == 'f' ? 30 : -30);
      commandTime = millis(); moving = true;
    }
  }
  if (moving && millis() - commandTime >= 1000) {
    RB.stop(motorChannel); moving = false;
  }
}

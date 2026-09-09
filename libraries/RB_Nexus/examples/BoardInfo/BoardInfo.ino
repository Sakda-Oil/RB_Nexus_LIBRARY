#include <RB_Nexus.h>

// Serial-only example: it does not configure motor, PWM or sensor GPIOs.
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("%s package %s\n", RBNexus::name, RBNexus::version);
  Serial.printf("Chip: %s, revision: %u\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("Flash: %u bytes, CPU: %u MHz\n", ESP.getFlashChipSize(), ESP.getCpuFreqMHz());
  Serial.println("Peripheral pin mapping: pending schematic");
}

void loop() {
  delay(1000);
}

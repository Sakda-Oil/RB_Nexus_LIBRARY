#include <RB_Nexus.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <LittleFS.h>
#include <Preferences.h>

#ifndef ARDUINO_RB_NEXUS
#error "Select RB_Nexus in Tools > Board for this package verification sketch."
#endif
static_assert(RBNexus::peripheralPinMapAvailable, "Peripheral pin mapping must be available.");

// Link the bundled core libraries and verify driver compilation
void setup() {
  Serial.begin(115200);
  Serial.println(RBNexus::name);
  Serial.println(RBNexus::version);
  Serial.println(WiFi.status());
  Serial.println(LittleFS.totalBytes());
  Preferences preferences;
  Serial.println(preferences.isKey("rb_nexus"));

  // Check RB driver symbols
  RB.setLED(false);
  RB.motorStopAll();
}

void loop() { delay(1000); }

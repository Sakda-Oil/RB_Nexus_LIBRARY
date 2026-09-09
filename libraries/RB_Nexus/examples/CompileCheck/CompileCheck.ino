#include <RB_Nexus.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <LittleFS.h>
#include <Preferences.h>

#ifndef ARDUINO_RB_NEXUS
#error "Select RB_Nexus in Tools > Board for this package verification sketch."
#endif
static_assert(!RBNexus::peripheralPinMapAvailable, "Review pin mapping before updating this test.");

// Link the bundled core libraries without starting peripheral buses or Wi-Fi.
void setup() {
  Serial.begin(115200);
  Serial.println(RBNexus::name);
  Serial.println(WiFi.status());
  Serial.println(LittleFS.totalBytes());
  Preferences preferences;
  Serial.println(preferences.isKey("rb_nexus"));
}
void loop() { delay(1000); }

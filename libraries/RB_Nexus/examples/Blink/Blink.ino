#include <RB_Nexus.h>
#include <driver/gpio.h>

// ไฟกระพริบสถานะ LED บนบอร์ด RB_Nexus V0.1 (GPIO 2)
// ติด 500 ms และดับ 500 ms
#ifndef RB_EXAMPLE_LED_PIN
#define RB_EXAMPLE_LED_PIN RB_PIN_LED
#endif
const int LED_PIN = RB_EXAMPLE_LED_PIN;
const bool LED_ACTIVE_LOW = false;  // LED บนบอร์ดติดเมื่อ GPIO เป็น HIGH
const unsigned long BLINK_INTERVAL_MS = 500;
bool ready = false;
bool ledOn = false;
unsigned long lastChange = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!GPIO_IS_VALID_OUTPUT_GPIO(LED_PIN) || (LED_PIN >= 6 && LED_PIN <= 11)) {
    Serial.println("Invalid LED GPIO.");
    return;
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
  lastChange = millis();
  ready = true;
  Serial.println("RB_Nexus - Blink (Status LED GPIO 2)");
}

void loop() {
  if (ready && millis() - lastChange >= BLINK_INTERVAL_MS) {
    lastChange = millis();
    ledOn = !ledOn;
    digitalWrite(LED_PIN, (ledOn != LED_ACTIVE_LOW) ? HIGH : LOW);
  }
  delay(1);
}

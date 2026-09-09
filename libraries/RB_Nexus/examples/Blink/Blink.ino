#include <RB_Nexus.h>
#include <driver/gpio.h>

// ไฟกระพริบ: ติด 500 ms และดับ 500 ms
// เปลี่ยน -1 เป็น GPIO ของ LED ที่ตรวจสอบจากวงจรจริงแล้ว
// ถ้าใช้ LED ภายนอก: GPIO -> ตัวต้านทาน 330 ohm -> ขา Anode (+)
// แล้วต่อขา Cathode (-) ของ LED ไป GND และใช้ LED_ACTIVE_LOW = false
// ยังไม่ทราบ GPIO ของ LED บน RB_Nexus จึงไม่กำหนดหมายเลขขาแทนให้
#ifndef RB_EXAMPLE_LED_PIN
#define RB_EXAMPLE_LED_PIN -1
#endif
const int LED_PIN = RB_EXAMPLE_LED_PIN;
const bool LED_ACTIVE_LOW = false;  // เปลี่ยนเป็น true ถ้า LED ติดเมื่อ GPIO เป็น LOW
const unsigned long BLINK_INTERVAL_MS = 500;
bool ready = false;
bool ledOn = false;
unsigned long lastChange = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!GPIO_IS_VALID_OUTPUT_GPIO(LED_PIN) || (LED_PIN >= 6 && LED_PIN <= 11)) {
    Serial.println("Set RB_EXAMPLE_LED_PIN to the verified LED GPIO first.");
    return;
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);
  lastChange = millis();
  ready = true;
  Serial.println("RB_Nexus - Blink");
}

void loop() {
  // ใช้ millis() เพื่อให้เพิ่มงานอื่นใน loop() ได้ระหว่างไฟกระพริบ
  if (ready && millis() - lastChange >= BLINK_INTERVAL_MS) {
    lastChange = millis();
    ledOn = !ledOn;
    digitalWrite(LED_PIN, (ledOn != LED_ACTIVE_LOW) ? HIGH : LOW);
  }
  delay(1);
}

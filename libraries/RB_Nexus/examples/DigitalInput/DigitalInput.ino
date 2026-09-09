#include <RB_Nexus.h>
#include <driver/gpio.h>

// เปลี่ยน -1 เป็น GPIO ที่ต่อกับสัญญาณดิจิทัลจริง (ไม่ใช่เลขช่องคอนเน็กเตอร์)
// ใช้ INPUT: ต้องมี pull-up/pull-down ภายนอกเพื่อไม่ให้ขาลอย
// สัญญาณเข้า GPIO ต้องอยู่ในช่วง 0-3.3 V
#ifndef RB_EXAMPLE_INPUT_PIN
#define RB_EXAMPLE_INPUT_PIN -1
#endif
const int INPUT_PIN = RB_EXAMPLE_INPUT_PIN;
bool ready = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!GPIO_IS_VALID_GPIO(INPUT_PIN) || (INPUT_PIN >= 6 && INPUT_PIN <= 11)) {
    Serial.println("Set RB_EXAMPLE_INPUT_PIN to a verified, available GPIO first.");
    return;
  }
  pinMode(INPUT_PIN, INPUT);
  ready = true;
  Serial.println("RB_Nexus - Digital Input (0=LOW, 1=HIGH)");
}

void loop() {
  if (ready) Serial.println(digitalRead(INPUT_PIN));
  delay(100);
}

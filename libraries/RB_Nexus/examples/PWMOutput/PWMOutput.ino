#include <RB_Nexus.h>
#include <driver/gpio.h>

// เปลี่ยน -1 เป็น GPIO output ที่ตรวจสอบแล้วว่าใช้ได้
// ใช้กับ LED ภายนอกผ่านตัวต้านทาน หรือเครื่องมือวัดสัญญาณ
// ตัวอย่างนี้เป็น PWM GPIO 5 kHz ไม่ใช่คำสั่งมอเตอร์หรือ Servo ของบอร์ด
#ifndef RB_EXAMPLE_PWM_PIN
#define RB_EXAMPLE_PWM_PIN -1
#endif
const int PWM_PIN = RB_EXAMPLE_PWM_PIN;
bool ready = false;
int duty = 0;
int step = 5;

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!GPIO_IS_VALID_OUTPUT_GPIO(PWM_PIN) || (PWM_PIN >= 6 && PWM_PIN <= 11)) {
    Serial.println("Set RB_EXAMPLE_PWM_PIN to a verified output GPIO first.");
    return;
  }
  // Arduino-ESP32 3.x ใช้ ledcAttach/ledcWrite โดยระบุหมายเลข GPIO
  if (!ledcAttach(PWM_PIN, 5000, 8)) {
    Serial.println("Cannot configure PWM.");
    return;
  }
  ledcWrite(PWM_PIN, 0);
  ready = true;
  Serial.println("RB_Nexus - PWM fade (duty 0-255)");
}

void loop() {
  if (ready) {
    if (!ledcWrite(PWM_PIN, duty)) {
      Serial.println("PWM write failed.");
      ledcDetach(PWM_PIN);
      pinMode(PWM_PIN, OUTPUT);
      digitalWrite(PWM_PIN, LOW);
      ready = false;
      return;
    }
    duty += step;
    if (duty >= 255) { duty = 255; step = -5; }
    if (duty <= 0) { duty = 0; step = 5; }
  }
  delay(20);
}

#include <RB_Nexus.h>

// เปลี่ยน -1 เป็น GPIO ADC1 ที่ต่อกับสัญญาณ 0-3.3 V จริง
// ESP32-WROOM-32: GPIO 32, 33, 34, 35, 36 หรือ 39
// นี่คือ ADC ภายใน ESP32 ไม่ใช่การแมปช่อง Analog 1-8 ของ RB_Nexus
#ifndef RB_EXAMPLE_ADC_PIN
#define RB_EXAMPLE_ADC_PIN -1
#endif
const int ADC_PIN = RB_EXAMPLE_ADC_PIN;
bool ready = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!(ADC_PIN == 32 || ADC_PIN == 33 || ADC_PIN == 34 || ADC_PIN == 35 ||
        ADC_PIN == 36 || ADC_PIN == 39)) {
    Serial.println("Set RB_EXAMPLE_ADC_PIN to an available ADC1 GPIO first.");
    return;
  }
  analogReadResolution(12);  // ค่าดิบ 0-4095 ไม่ใช่แรงดันที่สอบเทียบแล้ว
  ready = true;
  Serial.println("RB_Nexus - Analog Input (raw 12-bit ADC)");
}

void loop() {
  if (ready) Serial.println(analogRead(ADC_PIN));
  delay(100);
}

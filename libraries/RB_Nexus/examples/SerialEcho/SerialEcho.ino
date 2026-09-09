#include <RB_Nexus.h>

// เปิด Serial Monitor ที่ 115200 baud พิมพ์ข้อความแล้วกด Send
// ตัวอย่างนี้ส่งตัวอักษรที่ได้รับกลับไป ไม่ต้องต่ออุปกรณ์เพิ่ม
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("RB_Nexus - Serial Echo");
  Serial.println("Type a message and press Send.");
}

void loop() {
  while (Serial.available() > 0) {
    Serial.write(Serial.read());
  }
  delay(1);
}

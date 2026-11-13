#include <RB_Nexus.h>

// ตัวอย่างการอ่านปุ่มกดพร้อม Debounce บนบอร์ด RB_Nexus
// ใช้ขา Digital GPIO 4 (หรือเลือก 12, 14, 26, 27)
// ต่อปุ่มระหว่างขากับ GND; RB.buttonPressed() เปิด pull-up ภายในให้เอง
// กดหนึ่งครั้งจะได้ true หนึ่งครั้ง และสลับสถานะ LED บนบอร์ด (GPIO 2)

const int BUTTON_PIN = RB_PIN_D4;

void setup() {
  Serial.begin(115200);
  delay(1000);

  RB.begin();
  RB.led(false);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Button Debounce Example");
  Serial.printf("Connect Button between Pin D%d and GND.\n", BUTTON_PIN);
  Serial.println("=========================================");
}

void loop() {
  RB.update();

  if (RB.buttonPressed(BUTTON_PIN)) {
    RB.ledToggle();
    Serial.println("Button Pressed! Toggled Status LED.");
  }
}

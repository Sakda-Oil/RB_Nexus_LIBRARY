#include <RB_Nexus.h>

// ตัวอย่างการอ่านปุ่มกดพร้อม Debounce บนบอร์ด RB_Nexus
// ใช้ขา Digital GPIO 4 (หรือเลือก 12, 14, 26, 27)
// เมื่อกดปุ่ม (ต่อลง GND) จะสลับสถานะของ LED บนบอร์ด (GPIO 2)

const int BUTTON_PIN = RB_PIN_D4;
const unsigned long DEBOUNCE_DELAY_MS = 50;

int lastButtonState = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  RB.begin();
  RB.pinMode(BUTTON_PIN, INPUT_PULLUP);
  RB.setLED(false);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Button Debounce Example");
  Serial.printf("Connect Button between Pin D%d and GND.\n", BUTTON_PIN);
  Serial.println("=========================================");
}

void loop() {
  RB.update();

  int reading = RB.digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        RB.toggleLED();
        Serial.println("Button Pressed! Toggled Status LED.");
      }
    }
  }

  lastButtonState = reading;
}

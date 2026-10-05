#include <RB_Nexus.h>

// ทดสอบ Digital I/O บนบอร์ด RB_Nexus V0.1 (Pins: 4, 12, 14, 26, 27)
// ตามผลทดสอบ QC Pass รองรับทั้ง Input และ Output

const int DIGITAL_PINS[] = {RB_PIN_D4, RB_PIN_D12, RB_PIN_D14, RB_PIN_D26, RB_PIN_D27};
const int NUM_PINS = sizeof(DIGITAL_PINS) / sizeof(DIGITAL_PINS[0]);

bool outputTestMode = false;
unsigned long lastToggle = 0;
bool toggleState = false;

void printMenu() {
  Serial.println("\n--- RB_Nexus Digital I/O Test ---");
  Serial.println("Supported pins: 4, 12, 14, 26, 27");
  Serial.println("Commands:");
  Serial.println("  'i' - Mode: Input with Pull-up (Read status of pins)");
  Serial.println("  'o' - Mode: Output Toggle (Blinks pins HIGH/LOW every 1s)");
  Serial.println("  '1'-'5' - Set specific pin HIGH (when in output mode)");
  Serial.println("  '0' - Set all pins LOW");
}

void setModeInput() {
  outputTestMode = false;
  for (int i = 0; i < NUM_PINS; ++i) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }
  Serial.println("Switched to INPUT_PULLUP mode. Reading pins (1=HIGH/Open, 0=LOW/GND)...");
}

void setModeOutput() {
  outputTestMode = true;
  for (int i = 0; i < NUM_PINS; ++i) {
    pinMode(DIGITAL_PINS[i], OUTPUT);
    digitalWrite(DIGITAL_PINS[i], LOW);
  }
  Serial.println("Switched to OUTPUT mode. Toggling HIGH/LOW every 1 second...");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  RB.begin();
  RB.setLED(true);

  setModeInput();
  printMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'i' || cmd == 'I') {
      setModeInput();
    } else if (cmd == 'o' || cmd == 'O') {
      setModeOutput();
    } else if (cmd == '0') {
      for (int i = 0; i < NUM_PINS; ++i) digitalWrite(DIGITAL_PINS[i], LOW);
      Serial.println("All pins set to LOW.");
    } else if (cmd >= '1' && cmd <= '5') {
      int idx = cmd - '1';
      digitalWrite(DIGITAL_PINS[idx], HIGH);
      Serial.printf("Pin %d set to HIGH.\n", DIGITAL_PINS[idx]);
    } else if (cmd == 'h' || cmd == 'H' || cmd == '?') {
      printMenu();
    }
  }

  if (outputTestMode) {
    if (millis() - lastToggle >= 1000) {
      lastToggle = millis();
      toggleState = !toggleState;
      for (int i = 0; i < NUM_PINS; ++i) {
        digitalWrite(DIGITAL_PINS[i], toggleState ? HIGH : LOW);
      }
      RB.toggleLED();
      Serial.printf("Output pins [4, 12, 14, 26, 27] -> %s\n", toggleState ? "HIGH (3.3V)" : "LOW (GND)");
    }
  } else {
    // Input read mode - display periodically
    if (millis() - lastToggle >= 500) {
      lastToggle = millis();
      Serial.print("Digital Inputs: ");
      for (int i = 0; i < NUM_PINS; ++i) {
        int val = digitalRead(DIGITAL_PINS[i]);
        Serial.printf("D%d=%d  ", DIGITAL_PINS[i], val);
      }
      Serial.println();
    }
  }
  delay(10);
}

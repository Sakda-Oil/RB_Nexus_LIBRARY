#include <RB_Nexus.h>

// ทดสอบ Quadrature Encoders ทั้ง 4 ช่อง บนบอร์ด RB_Nexus V0.1
// ผังขา:
//   ENC1: A=GPIO 36 (SP), B=GPIO 39 (SN)
//   ENC2: A=GPIO 34,      B=GPIO 35
//   ENC3: A=GPIO 32,      B=GPIO 33
//   ENC4: A=GPIO 25,      B=GPIO 13

unsigned long lastPrint = 0;
int32_t lastTicks[4] = {0, 0, 0, 0};

void printMenu() {
  Serial.println("\n--- RB_Nexus Encoder Test ---");
  Serial.println("Channels:");
  Serial.println("  ENC1 (SP:36, SN:39) | ENC2 (34, 35)");
  Serial.println("  ENC3 (32, 33)       | ENC4 (25, 13)");
  Serial.println("Commands:");
  Serial.println("  'r' - Reset all encoder counts to 0");
  Serial.println("  '1'-'4' - Reset specific encoder");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  RB.begin();
  RB.encoderBegin();
  RB.setLED(true);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - 4-Channel Encoder Test");
  Serial.println("Rotate motor/encoder shafts to see ticks.");
  Serial.println("=========================================");
  printMenu();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'r' || cmd == 'R') {
      RB.resetAllEncoders();
      Serial.println("Reset all encoders to 0.");
    } else if (cmd >= '1' && cmd <= '4') {
      uint8_t id = cmd - '0';
      RB.resetEncoder(id);
      Serial.printf("Reset ENC%d to 0.\n", id);
    } else if (cmd == 'h' || cmd == 'H' || cmd == '?') {
      printMenu();
    }
  }

  if (millis() - lastPrint >= 200) {
    lastPrint = millis();

    int32_t t1 = RB.readEncoder(1);
    int32_t t2 = RB.readEncoder(2);
    int32_t t3 = RB.readEncoder(3);
    int32_t t4 = RB.readEncoder(4);

    // ตรวจจับว่ามีการหมุนหรือไม่
    bool moving = (t1 != lastTicks[0] || t2 != lastTicks[1] ||
                   t3 != lastTicks[2] || t4 != lastTicks[3]);

    if (moving) {
      RB.toggleLED();
    }

    Serial.printf("ENC1: %8ld | ENC2: %8ld | ENC3: %8ld | ENC4: %8ld\n",
                  t1, t2, t3, t4);

    lastTicks[0] = t1;
    lastTicks[1] = t2;
    lastTicks[2] = t3;
    lastTicks[3] = t4;
  }
}

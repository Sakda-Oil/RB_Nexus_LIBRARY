#include <RB_Nexus.h>

// ตัวอย่างระบบควบคุมความเร็วรอบมอเตอร์แบบวงรอบปิด (Closed-Loop PID Speed Control)
// ใช้คู่กับ Encoder เพื่อควบคุมความเร็วรอบ (RPM) ของมอเตอร์
// รองรับมอเตอร์ M1..M4

const uint8_t TARGET_MOTOR = 4; // ใช้มอเตอร์ M4 (ซึ่งผ่าน QC หมุนปกติ)
float targetRPM = 120.0f;       // เป้าหมาย 120 รอบต่อนาที

void setup() {
  Serial.begin(115200);
  delay(1000);

  RB.begin();
  RB.encoderBegin();
  RB.encoderSetPPR(TARGET_MOTOR, 330); // 330 ticks ต่อรอบมอเตอร์

  // ตั้งค่าพารามิเตอร์ PID (Kp, Ki, Kd)
  RB.motorSetPID(TARGET_MOTOR, 1.5f, 0.4f, 0.05f);

  // กำหนดความเร็วเป้าหมาย และเปิดระบบ PID
  RB.motorSetRPM(TARGET_MOTOR, targetRPM);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Closed-Loop PID Motor Speed Control");
  Serial.printf("Target Motor: M%d | Target RPM: %.1f\n", TARGET_MOTOR, targetRPM);
  Serial.println("Commands:");
  Serial.println("  '+' / '-' : Increase / Decrease target RPM");
  Serial.println("  's'       : Stop PID and Motor");
  Serial.println("=========================================");
}

void loop() {
  // สำคัญ: ต้องเรียก RB.update() ใน loop เสมอเพื่อคำนวณรอบการทำงานของ PID
  RB.update();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '+') {
      targetRPM += 20.0f;
      RB.motorSetRPM(TARGET_MOTOR, targetRPM);
      Serial.printf("New Target RPM: %.1f\n", targetRPM);
    } else if (c == '-') {
      targetRPM -= 20.0f;
      if (targetRPM < 0.0f) targetRPM = 0.0f;
      RB.motorSetRPM(TARGET_MOTOR, targetRPM);
      Serial.printf("New Target RPM: %.1f\n", targetRPM);
    } else if (c == 's' || c == 'S') {
      RB.motorPIDDisable(TARGET_MOTOR);
      Serial.println("PID Disabled. Motor Stopped.");
    }
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();
    float currentRPM = RB.encoderRPM(TARGET_MOTOR);
    int32_t ticks = RB.encoderRead(TARGET_MOTOR);
    Serial.printf("Target: %6.1f RPM | Actual: %6.1f RPM | Ticks: %8ld\n",
                  targetRPM, currentRPM, ticks);
    RB.toggleLED();
  }
}

#include <RB_Nexus.h>

// ทดสอบ Servo Output 8 ช่อง (Servo 1 ถึง 8 = PCA9685 CH8 ถึง CH15) บนบอร์ด RB_Nexus V0.1
// ผ่านชิป PCA9685 I2C (Address 0x40) ความถี่ 50 Hz
// ตามผลทดสอบ QC Pass

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Servo Sweep Test (Servo 1 - 8)");
  Serial.println("=========================================");

  bool ok = RB.begin();
  if (!ok) {
    Serial.println("WARNING: PCA9685 not detected at I2C 0x40!");
    Serial.println("Please check 6-24V power supply and I2C wiring.");
  }

  // Initial center position (90 degrees) for all servos 1-8
  for (uint8_t s = 1; s <= RB_SERVO_COUNT; ++s) {
    RB.servoAngle(s, 90);
  }
  delay(1000);
}

void loop() {
  Serial.println("\nMoving all servos to 0 degrees...");
  for (uint8_t s = 1; s <= RB_SERVO_COUNT; ++s) {
    RB.servoAngle(s, 0);
  }
  RB.led(true);
  delay(1500);

  Serial.println("Moving all servos to 90 degrees (Center)...");
  for (uint8_t s = 1; s <= RB_SERVO_COUNT; ++s) {
    RB.servoAngle(s, 90);
  }
  RB.led(false);
  delay(1500);

  Serial.println("Moving all servos to 180 degrees...");
  for (uint8_t s = 1; s <= RB_SERVO_COUNT; ++s) {
    RB.servoAngle(s, 180);
  }
  RB.led(true);
  delay(1500);

  Serial.println("Moving all servos to 90 degrees (Center)...");
  for (uint8_t s = 1; s <= RB_SERVO_COUNT; ++s) {
    RB.servoAngle(s, 90);
  }
  RB.led(false);
  delay(1500);
}

#include <RB_Nexus.h>

// ทดสอบ Servo Output 8 ช่อง (Channels 8 ถึง 15) บนบอร์ด RB_Nexus V0.1
// ผ่านชิป PCA9685 I2C (Address 0x40) ความถี่ 50 Hz
// ตามผลทดสอบ QC Pass

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Servo Sweep Test (Channels 8 - 15)");
  Serial.println("=========================================");

  bool ok = RB.begin();
  if (!ok) {
    Serial.println("WARNING: PCA9685 not detected at I2C 0x40!");
    Serial.println("Please check 6-24V power supply and I2C wiring.");
  }

  // Initial center position (90 degrees) for all servos 8-15
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) {
    RB.servo(ch, 90);
  }
  delay(1000);
}

void loop() {
  Serial.println("\nMoving all servos to 0 degrees...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) {
    RB.servo(ch, 0);
  }
  RB.setLED(true);
  delay(1500);

  Serial.println("Moving all servos to 90 degrees (Center)...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) {
    RB.servo(ch, 90);
  }
  RB.setLED(false);
  delay(1500);

  Serial.println("Moving all servos to 180 degrees...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) {
    RB.servo(ch, 180);
  }
  RB.setLED(true);
  delay(1500);

  Serial.println("Moving all servos to 90 degrees (Center)...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) {
    RB.servo(ch, 90);
  }
  RB.setLED(false);
  delay(1500);
}

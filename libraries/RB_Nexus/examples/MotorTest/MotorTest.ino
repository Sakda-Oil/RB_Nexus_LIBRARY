#include <RB_Nexus.h>

// ทดสอบ DC Motor 4 ช่อง (M1 - M4) บนบอร์ด RB_Nexus V0.1
// ขับผ่านชิป PCA9685 I2C (Address 0x40):
//   M1: CH0, CH1 (Whiteboard note: สั่งงานไม่ได้)
//   M2: CH2, CH3 (Whiteboard note: หมุนทิศทางเดียว)
//   M3: CH4, CH5 (Whiteboard note: สั่งงานไม่ได้)
//   M4: CH6, CH7 (Whiteboard note: หมุนปกติ)

int currentMotor = 1;
int testSpeed = 60; // 60%
bool autoTestRunning = false;
unsigned long autoTestTimer = 0;
int autoTestStep = 0;

void printMenu() {
  Serial.println("\n=========================================");
  Serial.println("RB_Nexus - DC Motor Test & Diagnostics");
  Serial.println("=========================================");
  Serial.println("Motors: M1 (CH0,1), M2 (CH2,3), M3 (CH4,5), M4 (CH6,7)");
  Serial.println("Commands:");
  Serial.println("  '1'-'4' : Select Motor M1, M2, M3, or M4");
  Serial.println("  'f'     : Forward (at current test speed)");
  Serial.println("  'b'     : Backward (at current test speed)");
  Serial.println("  's'     : Stop selected motor");
  Serial.println("  ' '     : STOP ALL MOTORS (Spacebar)");
  Serial.println("  '+'/'-' : Increase/Decrease test speed (+/- 10%)");
  Serial.println("  'a'     : Run Automated Sequence (tests M1 -> M4 forward & backward)");
  Serial.println("  'd'     : Diagnostic Mode (tests PCA9685 CH0 to CH7 individually)");
  Serial.printf("Selected: Motor %d | Speed: %d%%\n", currentMotor, testSpeed);
  Serial.println("=========================================");
}

void runDiagnostics() {
  Serial.println("\n--- PCA9685 Individual Pin Diagnostic ---");
  Serial.println("Testing channels 0 to 7 individually for 1 second each.");
  Serial.println("Use multimeter/probe to check signal on each pin:");
  for (uint8_t ch = 0; ch < 8; ++ch) {
    uint8_t mId = (ch / 2) + 1;
    char pinType = (ch % 2 == 0) ? 'A' : 'B';
    Serial.printf("Activating CH%d (Motor %d Pin %c) at 50%% PWM... ", ch, mId, pinType);
    RB.setPWMDuty(ch, 2048);
    RB.setLED(true);
    delay(1000);
    RB.setPWMDuty(ch, 0);
    RB.setLED(false);
    Serial.println("OFF");
    delay(200);
  }
  Serial.println("Diagnostic complete. All motor channels set to 0.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  bool ok = RB.begin();
  if (!ok) {
    Serial.println("WARNING: PCA9685 not detected at I2C 0x40!");
    Serial.println("Please check board power supply (6-24V DC) and I2C wiring.");
  } else {
    Serial.println("PCA9685 I2C communication OK.");
  }
  RB.motorStopAll();
  printMenu();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= '1' && c <= '4') {
      currentMotor = c - '0';
      Serial.printf("Selected Motor M%d\n", currentMotor);
    } else if (c == 'f' || c == 'F') {
      autoTestRunning = false;
      Serial.printf("Motor M%d -> FORWARD (%d%%)\n", currentMotor, testSpeed);
      RB.motor(currentMotor, testSpeed);
    } else if (c == 'b' || c == 'B') {
      autoTestRunning = false;
      Serial.printf("Motor M%d -> BACKWARD (-%d%%)\n", currentMotor, testSpeed);
      RB.motor(currentMotor, -testSpeed);
    } else if (c == 's' || c == 'S') {
      autoTestRunning = false;
      Serial.printf("Motor M%d -> STOP\n", currentMotor);
      RB.motorStop(currentMotor);
    } else if (c == ' ') {
      autoTestRunning = false;
      Serial.println("ALL MOTORS STOPPED.");
      RB.motorStopAll();
    } else if (c == '+') {
      testSpeed = min(100, testSpeed + 10);
      Serial.printf("Test speed: %d%%\n", testSpeed);
    } else if (c == '-') {
      testSpeed = max(10, testSpeed - 10);
      Serial.printf("Test speed: %d%%\n", testSpeed);
    } else if (c == 'a' || c == 'A') {
      autoTestRunning = true;
      autoTestStep = 0;
      autoTestTimer = millis();
      Serial.println("Starting Automated Test Sequence (M1 to M4)...");
    } else if (c == 'd' || c == 'D') {
      autoTestRunning = false;
      RB.motorStopAll();
      runDiagnostics();
    } else if (c == 'h' || c == 'H' || c == '?') {
      printMenu();
    }
  }

  // Automated Test Sequence
  if (autoTestRunning) {
    if (millis() - autoTestTimer >= 1500) {
      autoTestTimer = millis();
      RB.motorStopAll();

      int m = (autoTestStep / 2) + 1;
      bool forward = (autoTestStep % 2 == 0);

      if (m <= 4) {
        Serial.printf("[AUTO TEST] M%d: %s (%d%%)\n", m, forward ? "FORWARD" : "BACKWARD", testSpeed);
        RB.motor(m, forward ? testSpeed : -testSpeed);
        RB.toggleLED();
        autoTestStep++;
      } else {
        autoTestRunning = false;
        RB.motorStopAll();
        RB.setLED(false);
        Serial.println("[AUTO TEST] Finished sequence. All motors stopped.");
      }
    }
  }
}

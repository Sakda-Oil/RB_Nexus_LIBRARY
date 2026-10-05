#include <RB_Nexus.h>

// =============================================================================
// RB_Nexus V0.1 Full Board QC & Diagnostics Test
// =============================================================================
// โปรแกรมทดสอบรวมทุกระบบของบอร์ด RB Nexus V0.1:
//   1. Status LED (GPIO 2)
//   2. Digital I/O (Pins 4, 12, 14, 26, 27)
//   3. Encoders (ENC1, ENC2, ENC3, ENC4)
//   4. PCA9685 DC Motors (M1 - M4)
//   5. PCA9685 Servos (CH8 - CH15)
//   6. MCP3208 12-bit SPI ADC (CH1 - CH8)
//   7. I2C Bus Scanner
// =============================================================================

enum TestMode {
  MODE_IDLE,
  MODE_MONITOR_ENCODERS,
  MODE_MONITOR_ANALOG,
  MODE_MONITOR_DIGITAL_IN,
  MODE_DIGITAL_OUT_BLINK
};

TestMode currentMode = MODE_IDLE;
unsigned long lastAction = 0;
bool blinkState = false;

const int DIG_PINS[] = {RB_PIN_D4, RB_PIN_D12, RB_PIN_D14, RB_PIN_D26, RB_PIN_D27};
const int NUM_DIG_PINS = sizeof(DIG_PINS) / sizeof(DIG_PINS[0]);

void printMainMenu() {
  currentMode = MODE_IDLE;
  Serial.println("\n========================================================");
  Serial.println("         RB_Nexus V0.1 Full Board Test & QC Menu        ");
  Serial.println("========================================================");
  Serial.println("  [1] Test Status LED (GPIO 2)");
  Serial.println("  [2] Test Digital Outputs (Blink pins 4, 12, 14, 26, 27)");
  Serial.println("  [3] Monitor Digital Inputs (Pins 4, 12, 14, 26, 27)");
  Serial.println("  [4] Monitor Encoders (ENC1 - ENC4 Live Ticks)");
  Serial.println("  [5] Monitor Analog Inputs (MCP3208 CH1 - CH8)");
  Serial.println("  [6] Test DC Motors (M1 -> M4 Forward/Reverse Sequence)");
  Serial.println("  [7] Test Servos (Sweep CH8 - CH15: 0 -> 90 -> 180 deg)");
  Serial.println("  [8] Scan I2C Bus (Check PCA9685 & other connected chips)");
  Serial.println("  [d] Diagnostic: Motor Driver individual channels (CH0-CH7)");
  Serial.println("  [s] STOP / Clear all outputs");
  Serial.println("========================================================");
  Serial.print("Select an option [1-8, d, s]: ");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  bool pcaOk = RB.begin();
  RB.encoderBegin();

  Serial.println("\nInitializing RB_Nexus...");
  Serial.printf("PCA9685 I2C (0x40): %s\n", pcaOk ? "OK" : "NOT FOUND (Check 6-24V power)");

  printMainMenu();
}

void testLED() {
  Serial.println("\n[1] Testing Status LED (GPIO 2)...");
  for (int i = 0; i < 6; ++i) {
    RB.setLED(i % 2 == 0);
    delay(250);
  }
  RB.setLED(true);
  Serial.println("LED test finished (LED left ON).");
}

void startDigitalOutputTest() {
  Serial.println("\n[2] Testing Digital Outputs on pins 4, 12, 14, 26, 27...");
  Serial.println("Toggling pins HIGH/LOW every 1 sec. Press 's' to return to menu.");
  for (int i = 0; i < NUM_DIG_PINS; ++i) {
    pinMode(DIG_PINS[i], OUTPUT);
    digitalWrite(DIG_PINS[i], LOW);
  }
  currentMode = MODE_DIGITAL_OUT_BLINK;
  lastAction = millis();
}

void startDigitalInputMonitor() {
  Serial.println("\n[3] Monitoring Digital Inputs with internal pull-up...");
  Serial.println("Connect pins 4, 12, 14, 26, 27 to GND to see LOW (0). Press 's' to return to menu.");
  for (int i = 0; i < NUM_DIG_PINS; ++i) {
    pinMode(DIG_PINS[i], INPUT_PULLUP);
  }
  currentMode = MODE_MONITOR_DIGITAL_IN;
  lastAction = millis();
}

void startEncoderMonitor() {
  Serial.println("\n[4] Monitoring Encoders (ENC1..ENC4)...");
  Serial.println("Rotate shafts. Press 'r' to reset counts, 's' to return to menu.");
  currentMode = MODE_MONITOR_ENCODERS;
  lastAction = millis();
}

void startAnalogMonitor() {
  Serial.println("\n[5] Monitoring MCP3208 Analog Inputs (CH1..CH8 12-bit)...");
  Serial.println("Press 's' to return to menu.");
  currentMode = MODE_MONITOR_ANALOG;
  lastAction = millis();
}

void testMotors() {
  Serial.println("\n[6] Running DC Motor Sequence (M1 to M4)...");
  for (int m = 1; m <= 4; ++m) {
    Serial.printf("  - Testing M%d FORWARD (60%%)...\n", m);
    RB.motor(m, 60);
    delay(1000);
    RB.motorStop(m);
    delay(200);

    Serial.printf("  - Testing M%d BACKWARD (-60%%)...\n", m);
    RB.motor(m, -60);
    delay(1000);
    RB.motorStop(m);
    delay(300);
  }
  Serial.println("Motor test finished. All motors stopped.");
}

void testServos() {
  Serial.println("\n[7] Testing Servos CH8 to CH15...");
  Serial.println("Moving to 0 degrees...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) RB.servo(ch, 0);
  delay(1200);

  Serial.println("Moving to 90 degrees (Center)...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) RB.servo(ch, 90);
  delay(1200);

  Serial.println("Moving to 180 degrees...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) RB.servo(ch, 180);
  delay(1200);

  Serial.println("Moving back to 90 degrees...");
  for (uint8_t ch = RB_SERVO_CH_MIN; ch <= RB_SERVO_CH_MAX; ++ch) RB.servo(ch, 90);
  Serial.println("Servo test finished.");
}

void scanI2C() {
  Serial.println("\n[8] Scanning I2C Bus (SDA=21, SCL=22)...");
  int count = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Found device at address 0x%02X", addr);
      if (addr == RB_PCA9685_ADDR) Serial.print(" (PCA9685 PWM Controller)");
      Serial.println();
      count++;
    }
  }
  Serial.printf("Found %d device(s).\n", count);
}

void testMotorDiagnostic() {
  Serial.println("\n[d] Diagnosing PCA9685 Motor Channels (0 to 7 individually)...");
  for (uint8_t ch = 0; ch < 8; ++ch) {
    uint8_t m = (ch / 2) + 1;
    char leg = (ch % 2 == 0) ? 'A' : 'B';
    Serial.printf("  Activating CH%d (Motor %d leg %c) at 50%%... ", ch, m, leg);
    RB.setPWMDuty(ch, 2048);
    delay(800);
    RB.setPWMDuty(ch, 0);
    Serial.println("OFF");
    delay(150);
  }
  Serial.println("Channel diagnostic completed.");
}

void stopAll() {
  currentMode = MODE_IDLE;
  RB.motorStopAll();
  for (int i = 0; i < NUM_DIG_PINS; ++i) {
    pinMode(DIG_PINS[i], INPUT);
  }
  RB.setLED(false);
  Serial.println("\nAll outputs stopped.");
  printMainMenu();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '1': testLED(); printMainMenu(); break;
      case '2': startDigitalOutputTest(); break;
      case '3': startDigitalInputMonitor(); break;
      case '4': startEncoderMonitor(); break;
      case '5': startAnalogMonitor(); break;
      case '6': testMotors(); printMainMenu(); break;
      case '7': testServos(); printMainMenu(); break;
      case '8': scanI2C(); printMainMenu(); break;
      case 'd': case 'D': testMotorDiagnostic(); printMainMenu(); break;
      case 's': case 'S': stopAll(); break;
      case 'r': case 'R':
        if (currentMode == MODE_MONITOR_ENCODERS) {
          RB.resetAllEncoders();
          Serial.println("Encoders reset to 0.");
        }
        break;
      case 'h': case 'H': case '?':
        printMainMenu();
        break;
    }
  }

  // Periodic mode handlers
  if (currentMode == MODE_DIGITAL_OUT_BLINK) {
    if (millis() - lastAction >= 1000) {
      lastAction = millis();
      blinkState = !blinkState;
      for (int i = 0; i < NUM_DIG_PINS; ++i) {
        digitalWrite(DIG_PINS[i], blinkState ? HIGH : LOW);
      }
      RB.setLED(blinkState);
      Serial.printf("Digital Out [4, 12, 14, 26, 27] = %s\n", blinkState ? "HIGH" : "LOW");
    }
  } else if (currentMode == MODE_MONITOR_DIGITAL_IN) {
    if (millis() - lastAction >= 500) {
      lastAction = millis();
      Serial.print("Digital In: ");
      for (int i = 0; i < NUM_DIG_PINS; ++i) {
        Serial.printf("D%d=%d  ", DIG_PINS[i], digitalRead(DIG_PINS[i]));
      }
      Serial.println();
    }
  } else if (currentMode == MODE_MONITOR_ENCODERS) {
    if (millis() - lastAction >= 300) {
      lastAction = millis();
      Serial.printf("ENC1: %6ld | ENC2: %6ld | ENC3: %6ld | ENC4: %6ld\n",
                    RB.readEncoder(1), RB.readEncoder(2),
                    RB.readEncoder(3), RB.readEncoder(4));
    }
  } else if (currentMode == MODE_MONITOR_ANALOG) {
    if (millis() - lastAction >= 500) {
      lastAction = millis();
      Serial.print("MCP3208 Analog: ");
      for (uint8_t ch = 1; ch <= 8; ++ch) {
        Serial.printf("CH%d:%4u ", ch, RB.readAnalog(ch));
      }
      Serial.println();
    }
  }
}

#include <RB_Nexus.h>
#include <Wire.h>
#include <driver/gpio.h>

#ifndef RB_EXAMPLE_SDA_PIN
#define RB_EXAMPLE_SDA_PIN RB_PIN_I2C_SDA
#endif
#ifndef RB_EXAMPLE_SCL_PIN
#define RB_EXAMPLE_SCL_PIN RB_PIN_I2C_SCL
#endif
const int SDA_PIN = RB_EXAMPLE_SDA_PIN;
const int SCL_PIN = RB_EXAMPLE_SCL_PIN;
bool ready = false;

bool usablePin(int pin) {
  return GPIO_IS_VALID_OUTPUT_GPIO(pin) && !(pin >= 6 && pin <= 11);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!usablePin(SDA_PIN) || !usablePin(SCL_PIN) || SDA_PIN == SCL_PIN) {
    Serial.println("Invalid I2C SDA/SCL pins.");
    return;
  }
  ready = Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeOut(50);
  Serial.printf("RB_Nexus - I2C Scanner (SDA=%d, SCL=%d)\n", SDA_PIN, SCL_PIN);
  if (!ready) {
    Serial.println("I2C initialization failed.");
  }
}

void loop() {
  if (ready) {
    int found = 0;
    int errors = 0;
    Serial.println("\nScanning I2C bus...");
    // ตรวจเฉพาะช่วง address 7-bit ที่ไม่สงวนไว้
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
      Wire.beginTransmission(address);
      uint8_t result = Wire.endTransmission();
      if (result == 0) {
        Serial.printf("  - Found device at 0x%02X", address);
        if (address == RB_PCA9685_ADDR) {
          Serial.print(" (PCA9685 PWM Controller - Motors/Servos)");
        }
        Serial.println();
        ++found;
      } else if (result != 2) {
        ++errors;
      }
    }
    Serial.printf("Total devices found: %d, bus errors: %d\n", found, errors);
  }
  delay(5000);
}

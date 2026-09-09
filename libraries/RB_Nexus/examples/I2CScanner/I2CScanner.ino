#include <RB_Nexus.h>
#include <Wire.h>
#include <driver/gpio.h>

// เปลี่ยน -1 ทั้งสองค่าเป็น GPIO SDA/SCL ตามวงจรจริง
// อุปกรณ์ต้องมี GND ร่วมกัน และมีตัวต้านทาน pull-up SDA/SCL ไปที่ 3.3 V
#ifndef RB_EXAMPLE_SDA_PIN
#define RB_EXAMPLE_SDA_PIN -1
#endif
#ifndef RB_EXAMPLE_SCL_PIN
#define RB_EXAMPLE_SCL_PIN -1
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
    Serial.println("Set different, verified RB_EXAMPLE_SDA_PIN and RB_EXAMPLE_SCL_PIN first.");
    return;
  }
  ready = Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.setTimeOut(50);
  Serial.println(ready ? "RB_Nexus - I2C Scanner" : "I2C initialization failed.");
}

void loop() {
  if (ready) {
    int found = 0;
    int errors = 0;
    // ตรวจเฉพาะช่วง address 7-bit ที่ไม่สงวนไว้
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
      Wire.beginTransmission(address);
      uint8_t result = Wire.endTransmission();
      if (result == 0) {
        Serial.printf("Found I2C device at 0x%02X\n", address);
        ++found;
      } else if (result != 2) {
        ++errors;
      }
    }
    Serial.printf("Devices: %d, bus errors: %d\n", found, errors);
  }
  delay(5000);
}

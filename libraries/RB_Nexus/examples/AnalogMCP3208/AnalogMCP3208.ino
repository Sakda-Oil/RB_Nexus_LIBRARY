#include <RB_Nexus.h>

// ทดสอบ Analog Input บนบอร์ด RB_Nexus V0.1
// ผ่านชิป MCP3208 12-bit ADC (SPI Communication: MOSI=23, MISO=19, CLK=18, CS=5)
// คอนเน็กเตอร์ช่อง 1 ถึง 8 (CH1 - CH8) ให้ค่าความละเอียด 0 - 4095

const float VREF = 3.3f; // แรงดันอ้างอิงของ MCP3208 บนบอร์ด (3.3V)

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Analog Input (MCP3208 12-bit ADC)");
  Serial.println("Channels: CH1 - CH8 | SPI: MOSI=23, MISO=19, CLK=18, CS=5");
  Serial.println("=========================================");

  RB.begin();
  RB.setLED(true);
}

void loop() {
  Serial.println("\n--- MCP3208 Analog Readings ---");
  for (uint8_t ch = 1; ch <= 8; ++ch) {
    uint16_t raw = RB.readAnalog(ch);
    float voltage = (raw * VREF) / 4095.0f;
    Serial.printf("CH%d: %4u (%.3f V)   ", ch, raw, voltage);
    if (ch == 4) Serial.println(); // ขึ้นบรรทัดใหม่ทุก 4 ช่อง
  }
  Serial.println();

  RB.toggleLED();
  delay(500);
}

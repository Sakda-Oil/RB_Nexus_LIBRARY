#include <RB_Nexus.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("========================================\n");
  Serial.printf("%s package %s\n", RBNexus::name, RBNexus::version);
  Serial.printf("Hardware: RB Nexus V0.1\n");
  Serial.printf("MCU: %s (rev %u), CPU: %u MHz, Flash: %u bytes\n",
                ESP.getChipModel(), ESP.getChipRevision(),
                ESP.getCpuFreqMHz(), ESP.getFlashChipSize());
  Serial.printf("========================================\n");
  Serial.println("Peripheral Pin Mapping (RB Nexus V0.1):");
  Serial.printf("  - Status LED: GPIO %d\n", RB_PIN_LED);
  Serial.printf("  - Digital I/O: GPIO 4, 12, 14, 26, 27\n");
  Serial.printf("  - Encoders:\n");
  Serial.printf("      ENC1: A=GPIO %d (SP), B=GPIO %d (SN)\n", RB_PIN_ENC1_A, RB_PIN_ENC1_B);
  Serial.printf("      ENC2: A=GPIO %d, B=GPIO %d\n", RB_PIN_ENC2_A, RB_PIN_ENC2_B);
  Serial.printf("      ENC3: A=GPIO %d, B=GPIO %d\n", RB_PIN_ENC3_A, RB_PIN_ENC3_B);
  Serial.printf("      ENC4: A=GPIO %d, B=GPIO %d\n", RB_PIN_ENC4_A, RB_PIN_ENC4_B);
  Serial.printf("  - I2C (PCA9685): SDA=GPIO %d, SCL=GPIO %d (Addr 0x%02X)\n",
                RB_PIN_I2C_SDA, RB_PIN_I2C_SCL, RB_PCA9685_ADDR);
  Serial.printf("      Motors M1..M4: PCA9685 CH0..CH7\n");
  Serial.printf("      Servos 8..15:  PCA9685 CH8..CH15\n");
  Serial.printf("  - SPI (MCP3208 12-bit ADC): MOSI=%d, MISO=%d, CLK=%d, CS=%d\n",
                RB_PIN_SPI_MOSI, RB_PIN_SPI_MISO, RB_PIN_SPI_CLK, RB_PIN_SPI_CS);
  Serial.printf("      Analog Inputs CH1..CH8: MCP3208 CH0..CH7 (0-4095)\n");
  Serial.printf("========================================\n");

  bool pcaFound = RB.begin();
  Serial.printf("PCA9685 I2C Status: %s\n", pcaFound ? "CONNECTED (OK)" : "NOT DETECTED (Check 6-24V power/wiring)");
}

void loop() {
  delay(1000);
}

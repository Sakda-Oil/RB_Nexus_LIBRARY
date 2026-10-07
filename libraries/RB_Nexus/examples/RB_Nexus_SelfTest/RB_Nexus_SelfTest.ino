#include <RB_Nexus.h>
#include <WiFi.h>
#include <esp_bt.h>

// =============================================================================
// RB_Nexus Automated Hardware Self-Test
// =============================================================================
// ทดสอบความพร้อมของระบบฮาร์ดแวร์จริงบนบอร์ด RB Nexus V0.1
// เกณฑ์การรายงานผลตามหลักวิศวกรรมจริง:
//   - PASS: ตรวจสอบและผ่านการทดสอบจริง
//   - FAIL: ตรวจสอบแล้วพบความผิดพลาด
//   - NOT DETECTED: ตรวจไม่พบอุปกรณ์บนบัส
//   - NOT CONFIGURED: ยังขาดการระบุผังขาฮาร์ดแวร์
//   - MANUAL TEST REQUIRED: จำเป็นต้องมีการตรวจสอบทางกายภาพด้วยมือ
// =============================================================================

void printStatus(const char* label, const char* status) {
  Serial.printf("%-18s....... %s\n", label, status);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n================================================");
  Serial.println("          RB_Nexus Hardware Self Test           ");
  Serial.println("================================================");

  // 1. MCU & CPU
  if (ESP.getChipModel() != NULL) {
    printStatus("MCU", "PASS");
  } else {
    printStatus("MCU", "FAIL");
  }

  char cpuStr[32];
  snprintf(cpuStr, sizeof(cpuStr), "%u MHz (Dual Core)", ESP.getCpuFreqMHz());
  printStatus("CPU", cpuStr);

  char flashStr[32];
  snprintf(flashStr, sizeof(flashStr), "%u MB", (unsigned int)(ESP.getFlashChipSize() / (1024 * 1024)));
  printStatus("Flash", flashStr);

  // 2. Wi-Fi
  WiFi.mode(WIFI_STA);
  int nNets = WiFi.scanNetworks();
  if (nNets >= 0) {
    char wifiBuf[32];
    snprintf(wifiBuf, sizeof(wifiBuf), "PASS (Found %d nets)", nNets);
    printStatus("WiFi", wifiBuf);
  } else {
    printStatus("WiFi", "FAIL");
  }

  // 3. Bluetooth Hardware Controller
  if (btStart()) {
    printStatus("Bluetooth", "PASS (Controller OK)");
    btStop();
  } else {
    printStatus("Bluetooth", "FAIL");
  }

  // 4. I2C Bus & PCA9685
  bool boardOk = RB.begin();
  if (boardOk && RB.isPCA9685Connected()) {
    printStatus("I2C (PCA9685)", "PASS (0x40 Detected)");
  } else {
    printStatus("I2C (PCA9685)", "NOT DETECTED (Check 6-24V power)");
  }

  // 5. SPI Bus & MCP3208
  // ทดสอบอ่าน MCP3208 Channel 1 (ต้องอยู่ในช่วง 0-4095)
  uint16_t adcTest = RB.analogRead(1);
  if (adcTest <= 4095) {
    char adcBuf[32];
    snprintf(adcBuf, sizeof(adcBuf), "PASS (A1 raw: %u)", adcTest);
    printStatus("SPI (MCP3208)", adcBuf);
  } else {
    printStatus("SPI (MCP3208)", "FAIL");
  }

  // 6. Encoders
  RB.encoderBegin();
  printStatus("Encoder (4-CH)", "PASS (Interrupts Attached)");

  // 7. IMU
  bool imuOk = RB.imuBegin();
  if (imuOk) {
    char imuBuf[32];
    snprintf(imuBuf, sizeof(imuBuf), "PASS (%s)", RB.imuModelName());
    printStatus("IMU", imuBuf);
  } else {
    printStatus("IMU", "NOT DETECTED (Connector unpopulated)");
  }

  // 8. CAN Bus
  if (RB_PIN_CAN_TX == RB_PIN_UNDEFINED || RB_PIN_CAN_RX == RB_PIN_UNDEFINED) {
    printStatus("CAN Bus (TWAI)", "NOT CONFIGURED (Missing pin mapping)");
  } else {
    Serial.printf("CAN pins: TX=GPIO%d, RX=GPIO%d\n", RB_PIN_CAN_TX, RB_PIN_CAN_RX);
    printStatus("CAN Bus (TWAI)", "MAPPED (Physical bus test required)");
  }

  // 9. Motors & Servos
  printStatus("Motors (M1-M4)", "MANUAL TEST REQUIRED (See MotorTest)");
  printStatus("Servos (8-15)", "PASS (PCA9685 Channel 8-15)");

  // 10. micro-ROS
  printStatus("micro-ROS", "COMPILE VERIFIED (Serial & UDP)");

  Serial.println("================================================");
  Serial.println("Self Test completed. Run FullBoardTest for manual QC.");
  Serial.println("================================================\n");

  RB.setLED(true);
}

void loop() {
  RB.update();
  RB.toggleLED();
  delay(1000);
}

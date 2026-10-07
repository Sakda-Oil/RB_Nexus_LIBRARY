#include <RB_Nexus.h>

// ตัวอย่างการใช้งาน CAN Bus (TWAI) บนบอร์ด RB_Nexus
// หมายเหตุทางวิศวกรรม:
//   ขาตามผังวงจร: TX=GPIO17, RX=GPIO16
//   ตัวอย่างนี้แสดงการตั้งค่าและรับส่งข้อมูล CAN Bus (500 kbps)
//   หากต่อ Transceiver ภายนอก ให้ระบุขา CAN_TX และ CAN_RX ตามที่ใช้งานจริง

// ใช้ขาของบอร์ดเป็นค่าเริ่มต้น หรือ override เมื่อต่อ Transceiver ภายนอก
// ตัวอย่างนี้ใช้ normal mode ต้องมี CAN node อีกตัวรับและ ACK พร้อม termination
// ชื่อ CANLoopback คงไว้เพื่อความเข้ากันได้ ไม่ใช่ internal self-reception test
#ifndef CUSTOM_CAN_TX
#define CUSTOM_CAN_TX RB_PIN_CAN_TX
#endif
#ifndef CUSTOM_CAN_RX
#define CUSTOM_CAN_RX RB_PIN_CAN_RX
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("RB_Nexus - CAN Bus (TWAI) Test");
  Serial.println("Pin mapping: schematic checked; physical CAN bus test required.");
  Serial.printf("Configuring CAN at 500 kbps (TX=%d, RX=%d)...\n", CUSTOM_CAN_TX, CUSTOM_CAN_RX);
  Serial.println("=========================================");

  bool ok = RB.canBegin(500000, CUSTOM_CAN_TX, CUSTOM_CAN_RX);
  if (!ok) {
    Serial.println("CAN Controller initialization failed.");
  } else {
    Serial.println("CAN Controller started successfully.");
  }
}

void loop() {
  RB.update();

  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 1000) {
    lastSend = millis();

    uint8_t payload[8] = {0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD};
    bool sent = RB.canSend(0x123, payload, 8);
    if (sent) {
      Serial.println("[CAN TX] Frame sent: ID 0x123, DLC 8");
      RB.toggleLED();
    }
  }

  uint32_t recvId = 0;
  uint8_t recvData[8];
  uint8_t recvLen = 0;
  bool isExt = false;

  if (RB.canReceive(recvId, recvData, recvLen, isExt)) {
    Serial.printf("[CAN RX] ID: 0x%X (Ext: %d), DLC: %d, Data: ", recvId, isExt, recvLen);
    for (uint8_t i = 0; i < recvLen; ++i) {
      Serial.printf("%02X ", recvData[i]);
    }
    Serial.println();
  }
}

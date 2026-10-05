#include <RB_Nexus.h>
#include <BluetoothSerial.h>

// ตัวอย่างการใช้งาน Bluetooth Classic Serial (SPP) บนบอร์ด RB_Nexus
// MCU: ESP32-WROOM-32 รองรับ Bluetooth Classic 4.2 BR/EDR
// สามารถใช้สมาร์ทโฟน (Android/PC) เชื่อมต่อผ่าน Bluetooth Serial Terminal

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  delay(1000);

  RB.begin();
  RB.setLED(true);

  SerialBT.begin("RB_Nexus_BT"); // ชื่ออุปกรณ์ Bluetooth ที่แสดงเมื่อค้นหา

  Serial.println("=========================================");
  Serial.println("RB_Nexus - Bluetooth Serial Echo");
  Serial.println("Device Name: RB_Nexus_BT");
  Serial.println("Pair and connect using Bluetooth Serial Terminal app.");
  Serial.println("=========================================");
}

void loop() {
  RB.update();

  // รับข้อมูลจาก Bluetooth ส่งออก Serial Monitor
  if (SerialBT.available()) {
    char c = SerialBT.read();
    Serial.write(c);
    // Echo กลับไปยังอุปกรณ์ Bluetooth
    SerialBT.write(c);
    RB.toggleLED();
  }

  // รับข้อมูลจาก Serial Monitor ส่งออก Bluetooth
  if (Serial.available()) {
    char c = Serial.read();
    SerialBT.write(c);
  }
}

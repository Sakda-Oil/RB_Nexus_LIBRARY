#include <RB_Nexus.h>
#include <WiFi.h>

// สแกนเครือข่าย Wi-Fi 2.4 GHz รอบบอร์ดทุก 10 วินาที
// ไม่ต้องใส่รหัสผ่านและไม่ได้เชื่อมต่อเครือข่าย
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);
  Serial.println("RB_Nexus - Wi-Fi Scan");
}

void loop() {
  Serial.println("Scanning...");
  int count = WiFi.scanNetworks();
  if (count < 0) {
    Serial.println("Scan failed; retrying in 10 seconds.");
  } else if (count == 0) {
    Serial.println("No networks found.");
  } else {
    for (int i = 0; i < count; ++i) {
      Serial.printf("%2d | %-32s | %4d dBm | channel %d | %s\n",
                    i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
                    WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "secured");
    }
  }
  WiFi.scanDelete();
  delay(10000);
}

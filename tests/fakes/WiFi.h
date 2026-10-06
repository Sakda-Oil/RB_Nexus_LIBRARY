#pragma once
#define WL_CONNECTED 3
#define WIFI_OFF 0
#define WIFI_STA 1
struct FakeIP { String toString() { return "127.0.0.1"; } };
struct FakeWiFi {
  int status() { return WL_CONNECTED; }
  int getMode() { return WIFI_STA; }
  void reconnect() {}
  void mode(int) {}
  void begin(const char*,const char*) {}
  int RSSI() { return -50; }
  FakeIP localIP() { return {}; }
};
inline FakeWiFi WiFi;

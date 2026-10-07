#pragma once
#include <vector>
#define SH2_ACCELEROMETER 1
#define SH2_GYROSCOPE_CALIBRATED 2
#define SH2_MAGNETIC_FIELD_CALIBRATED 3
#define SH2_ROTATION_VECTOR 4
#define SH2_OK 0
struct sh2_SensorValue_t {
  int sensorId;
  uint64_t timestamp;
  struct {
    struct { float x,y,z; } accelerometer, gyroscope, magneticField;
    struct { float real,i,j,k; } rotationVector;
  } un;
};
struct sh2_SensorEvent { sh2_SensorValue_t value; };
using sh2_SensorEvent_t = sh2_SensorEvent;
using sh2_SensorCallback_t = void(void*, sh2_SensorEvent_t*);
inline sh2_SensorCallback_t* fakeSensorCallback = nullptr;
inline void* fakeSensorCookie = nullptr;
inline bool fakeBNODecodeOK = true, fakeBNOResetOnService = false;
inline bool fakeBNOFound=false, fakeBNOReset=false, fakeBNOEnable=true;
inline std::vector<sh2_SensorValue_t> fakeEvents;
inline int sh2_setSensorCallback(sh2_SensorCallback_t* callback, void* cookie) {
  fakeSensorCallback = callback; fakeSensorCookie = cookie; return SH2_OK;
}
inline int sh2_decodeSensorEvent(sh2_SensorValue_t* value, const sh2_SensorEvent_t* event) {
  if (!fakeBNODecodeOK) return -1;
  *value = event->value; return SH2_OK;
}
inline void sh2_service() {
  if (fakeBNOResetOnService) { fakeBNOReset=true; fakeBNOResetOnService=false; return; }
  // One SHTP transfer can deliver multiple reports to the callback.
  const auto packet = fakeEvents; fakeEvents.clear();
  for (const auto& value : packet) {
    sh2_SensorEvent_t event{value};
    if (fakeSensorCallback) fakeSensorCallback(fakeSensorCookie, &event);
  }
}
class Adafruit_BNO08x {
public:
  explicit Adafruit_BNO08x(int) {}
  bool begin_I2C(uint8_t, FakeWire*) {
    fakeSensorCallback=nullptr; fakeSensorCookie=nullptr; return fakeBNOFound;
  }
  bool enableReport(int, uint32_t) { return fakeBNOEnable; }
  bool wasReset() { bool r=fakeBNOReset; fakeBNOReset=false; return r; }
  bool getSensorEvent(sh2_SensorValue_t* event) {
    if (fakeEvents.empty()) return false;
    // Match the vendor wrapper: the final report overwrites earlier reports.
    *event=fakeEvents.back(); fakeEvents.clear(); return true;
  }
};

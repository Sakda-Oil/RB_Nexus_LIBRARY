#pragma once
#include <vector>
#define SH2_ACCELEROMETER 1
#define SH2_GYROSCOPE_CALIBRATED 2
#define SH2_MAGNETIC_FIELD_CALIBRATED 3
#define SH2_ROTATION_VECTOR 4
struct sh2_SensorValue_t {
  int sensorId;
  uint64_t timestamp;
  struct {
    struct { float x,y,z; } accelerometer, gyroscope, magneticField;
    struct { float real,i,j,k; } rotationVector;
  } un;
};
inline bool fakeBNOFound=false, fakeBNOReset=false, fakeBNOEnable=true;
inline std::vector<sh2_SensorValue_t> fakeEvents;
class Adafruit_BNO08x {
public:
  explicit Adafruit_BNO08x(int) {}
  bool begin_I2C(uint8_t, FakeWire*) { return fakeBNOFound; }
  bool enableReport(int, uint32_t) { return fakeBNOEnable; }
  bool wasReset() { bool r=fakeBNOReset; fakeBNOReset=false; return r; }
  bool getSensorEvent(sh2_SensorValue_t* event) {
    if (fakeEvents.empty()) return false;
    *event=fakeEvents.front(); fakeEvents.erase(fakeEvents.begin()); return true;
  }
};

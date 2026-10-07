#include "RB_Nexus.h"
#include <MPU9250.h>
#include <Adafruit_BNO08x.h>
#include <cassert>
#include <cstdio>

int main() {
  RB.begin();
  assert(!RB.imuBegin(RBIMUType::MPU9250)); // ACK but wrong identity
  Wire.registers[0x75]=0x71;
  assert(RB.imuBegin(RBIMUType::MPU9250,0x68));
  assert(!RB.imuDataFresh());
  fakeMPUNew=true; assert(RB.imuUpdate());
  assert(RB.imuDataFresh() && RB.imuOrientationFresh());
  assert(fabsf(RB.accelZ()-9.80665f)<0.0001f);
  assert(fabsf(RB.gyroX()-3.14159265f)<0.0001f);
  assert(fabsf(RB.magY()-10)<0.0001f);
  fakeNow+=501; assert(!RB.imuDataFresh());
  puts("PASS MPU identity, SI units and stale-data detection");

  fakeBNOFound=true;
  assert(RB.imuBegin()); assert(RB.imuType()==RBIMUType::BNO085);
  sh2_SensorValue_t a={}; a.sensorId=SH2_ACCELEROMETER; a.un.accelerometer.z=9.81f;
  fakeEvents.push_back(a); RB.imuUpdate(); assert(!RB.imuDataFresh());
  sh2_SensorValue_t g={}; g.sensorId=SH2_GYROSCOPE_CALIBRATED; g.un.gyroscope.x=1;
  fakeEvents.push_back(g); RB.imuUpdate(); assert(RB.imuDataFresh());
  assert(!RB.imuOrientationFresh());
  sh2_SensorValue_t q={}; q.sensorId=SH2_ROTATION_VECTOR; q.un.rotationVector.real=2;
  fakeEvents.push_back(q); RB.imuUpdate();
  assert(RB.imuOrientationFresh() && RB.quaternionW()==1);
  assert(fabsf(RB.roll())<0.001f);
  fakeNow+=501; fakeEvents.push_back(g); RB.imuUpdate();
  assert(!RB.imuDataFresh()); // A stream of gyro events cannot refresh old acceleration.
  puts("PASS BNO selection, independent report freshness and quaternion normalization");

  fakeBNOReset=true; RB.imuUpdate(); assert(!RB.imuDataFresh());
  fakeEvents.push_back(a); fakeEvents.push_back(g); RB.imuUpdate(); assert(RB.imuDataFresh());
  fakeBNOReset=true; fakeBNOEnable=false; RB.imuUpdate(); assert(!RB.isIMUAvailable());
  assert(!RB.imuBegin(RBIMUType::MPU9250,0x4A));
  puts("PASS BNO reset recovery, report failure and explicit address validation");

  fakeBNOFound=false; Wire.registers[0x75]=0x68;
  assert(RB.imuBegin()); assert(RB.imuType()==RBIMUType::MPU6050);
  assert(Wire.registers[0x1B]==0x08 && Wire.registers[0x1C]==0x08);
  assert(!RB.imuDataFresh() && !RB.imuOrientationFresh());
  Wire.registers[0x3A]=1;
  Wire.registers[0x3B]=0xE0; Wire.registers[0x3C]=0; // -1g at +/-4g
  Wire.registers[0x43]=0x2E; Wire.registers[0x44]=0x0E; // +180 deg/s
  assert(RB.imuUpdate()); assert(RB.imuDataFresh());
  assert(fabsf(RB.accelX()+9.80665f)<0.0001f);
  assert(fabsf(RB.gyroX()-3.14159265f)<0.0001f);
  assert(!RB.imuOrientationFresh() && isnan(RB.magX()) && isnan(RB.yaw()));
  fakeNow+=501; Wire.shortRead=0;
  assert(!RB.imuUpdate() && !RB.imuDataFresh());
  Wire.shortRead = -1;
  Wire.registers[0x75]=0x70; // MPU6500 identity, not proof of module authenticity
  assert(RB.imuBegin(RBIMUType::Auto));
  assert(RB.imuType()==RBIMUType::MPU6500);
  assert(std::string(RB.imuModelName()) == "MPU6500 (6-axis)");
  assert(RB.imuBegin(RBIMUType::MPU9250)); // fallback when labeled MPU9250 but chip is MPU6500
  assert(RB.imuType()==RBIMUType::MPU6500);
  assert(RB.imuBegin(RBIMUType::MPU6500,0x69));
  assert(Wire.registers[0x1D]==0x03);
  assert(RB.imuUpdate() && RB.imuDataFresh());
  assert(fabsf(RB.accelX()+9.80665f)<0.0001f);
  assert(fabsf(RB.gyroX()-3.14159265f)<0.0001f);
  assert(!RB.imuOrientationFresh() && isnan(RB.magX()) && isnan(RB.quaternionW()));
  Wire.registers[0x3A]=0; fakeNow+=501;
  assert(!RB.imuUpdate() && !RB.imuDataFresh());
  Wire.registers[0x75]=0x68;
  assert(!RB.imuBegin(RBIMUType::MPU6500));
  Wire.registers[0x75]=0x72;
  assert(!RB.imuBegin()); // Do not accept unverified WHO_AM_I values.
  Wire.registers[0x75]=0x73;
  assert(RB.imuBegin(RBIMUType::MPU9250));
  assert(std::string(RB.imuModelName()) == "MPU9255 + AK8963");
  fakeMPUSetup=false;
  assert(!RB.imuBegin()); // A missing magnetometer is not a verified 9-axis device.
  fakeMPUSetup=true;

  puts("PASS MPU6050 and MPU6500 auto detection, ranges, signed SI conversion and failed reads");

  Wire.registers[0x75]=0x70; Wire.registers[0x3A]=1;
  assert(RB.imuBegin(RBIMUType::MPU6500));
  assert(isnan(RB.imuRotationZ()) && !RB.imuRotationZFresh());
  Wire.registers[0x47]=0x17; Wire.registers[0x48]=0x07; // 5895 = 90 deg/s
  RB.imuUpdate();
  for (int i=0; i<50; ++i) { fakeNow+=20; RB.imuUpdate(); }
  assert(fabsf(RB.imuRotationZ()-90)<0.01f && RB.imuRotationZFresh());
  assert(!RB.imuOrientationFresh() && isnan(RB.yaw()));
  assert(!RB.imuResetRotationZ(NAN) && !RB.imuSetGyroZBias(INFINITY));
  assert(RB.imuResetRotationZ(10));
  assert(RB.imuSetGyroZBias(90*DEG_TO_RAD));
  RB.imuUpdate(); fakeNow+=100; RB.imuUpdate();
  assert(fabsf(RB.imuRotationZ()-10)<0.01f);
  RB.imuSetGyroZBias(0); RB.imuResetRotationZ();
  Wire.registers[0x47]=0xE8; Wire.registers[0x48]=0xF9; // -90 deg/s
  RB.imuUpdate(); fakeNow+=100; RB.imuUpdate();
  assert(fabsf(RB.imuRotationZ()+9)<0.01f);
  Wire.registers[0x3A]=0; fakeNow+=501;
  assert(!RB.imuUpdate() && !RB.imuRotationZFresh());
  Wire.registers[0x3A]=1; RB.imuUpdate(); // Gap is skipped, not extrapolated.
  assert(fabsf(RB.imuRotationZ()+9)<0.01f);
  fakeNow=4294960; RB.imuResetRotationZ(); RB.imuUpdate();
  fakeNow+=20; RB.imuUpdate(); // micros() wraps here
  assert(fabsf(RB.imuRotationZ()+1.8f)<0.01f);
  puts("PASS relative Z degrees, bias, reset, direction, stale gaps and timer wrap");

  fakeBNOFound=true; fakeBNOEnable=true;
  assert(RB.imuBegin(RBIMUType::BNO085));
  g.un.gyroscope.z=90*DEG_TO_RAD;
  g.timestamp=1000000; fakeEvents.push_back(g);
  g.timestamp=1100000; fakeEvents.push_back(g);
  RB.imuUpdate(); // Both reports received in the same host millisecond.
  assert(fabsf(RB.imuRotationZ()-9)<0.01f);
  fakeBNOReset=true; RB.imuUpdate(); assert(!RB.imuRotationZFresh());
  puts("PASS BNO queued gyro sensor timestamps and reset freshness");
}

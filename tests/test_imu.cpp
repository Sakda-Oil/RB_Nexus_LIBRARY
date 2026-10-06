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
  puts("PASS MPU6050 auto detection, ranges, signed SI conversion and failed reads");
}

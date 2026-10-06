#include "RB_Nexus.h"
#include "RB_Nexus_Safety.h"
#include <driver/twai.h>
#include <cassert>
#include <cstdio>

// This test targets the production motor/PWM/CAN core; IMU is separately compiled.
bool RBNexusBoard::imuUpdate() { return false; }
uint16_t off(int channel) {
  int reg=0x06+4*channel;
  return Wire.registers[reg+2] | (Wire.registers[reg+3]<<8);
}
uint16_t on(int channel) {
  int reg=0x06+4*channel;
  return Wire.registers[reg] | (Wire.registers[reg+1]<<8);
}
int main() {
  assert(&RB == &RB_Nexus);
  assert(RB.begin());
  RB.motorSetRPM(1,100);
  fakeNow += 60; RB.update();
  assert(RB.isMotorPIDEnabled(1));
  assert(off(0)!=4096);
  RB.motorStop(1);
  fakeNow += 60; RB.update();
  assert(!RB.isMotorPIDEnabled(1));
  assert(off(0)==4096 && off(1)==4096);
  puts("PASS PID stop remains stopped after update");

  RB.motorSetRPM(1,100);
  RB.motorSet(1,80);
  assert(!RB.isMotorPIDEnabled(1));
  RB.motorSetRPM(1,0);
  assert(off(0)==4096);
  RB.motorSet(1,255); assert(on(0)==4096);
  RB.motorSet(1,256); assert(on(0)==4096);
  RB.motorSet(1,-32768); assert(on(1)==4096);
  puts("PASS manual override, zero RPM and speed saturation");

  RB.emergencyStop();
  RB_Nexus.motorSet(1,200); RB.motorBrake(1);
  assert(off(0)==4096 && off(1)==4096);
  RB.motorSetRPM(1,200); assert(!RB.isMotorPIDEnabled(1));
  RB.clearEmergencyStop();
  RB.motorSet(1,100); assert(off(0)!=4096);
  puts("PASS emergency latch shared by aliases");

  RBMotorCommandGuard guard(500);
  guard.feed(); fakeNow+=499; guard.update(); assert(off(0)!=4096);
  fakeNow++; guard.update(); assert(off(0)==4096);
  fakeNow=0xFFFFFF00; RB.motorSet(1,100); guard.feed();
  fakeNow+=501; guard.update(); assert(off(0)==4096);
  puts("PASS command timeout including millis rollover");

  RB.motorSet(1,100); RB.motorStop(1);
  uint32_t stopped=millis(); RB.motorSet(1,-100);
  assert(millis()-stopped>=5);
  RB.enableWatchdog(100); RB.motorSetRPM(1,100);
  fakeNow+=101; RB.update(); assert(RB.isEmergencyStopped());
  assert(off(0)==4096 && off(1)==4096);
  RB.disableWatchdog(); RB.clearEmergencyStop();
  puts("PASS stop/reverse dead time and watchdog");

  RB.setPWMFreq(100); RB.servoWriteMicroseconds(8,1500);
  assert(off(8)>=600 && off(8)<=625); // 1500 us at actual prescaler frequency
  RB.servoWriteMicroseconds(8,0); assert(off(8)==4096);
  RB.pwmWrite(4,500,5000,10); RB.pwmWrite(4,600,5000,10);
  assert(fakeDuty[4]==600);
  RB.pwmSetFrequency(4,1000); assert(fakeResolution[4]==10);
  puts("PASS servo frequency and repeated GPIO PWM writes");

  assert(RB.canBegin(500000,4,5));
  uint8_t data=42;
  assert(RB.canSend(0x123,&data,1));
  assert(!fakeSent.ss && !fakeSent.self && !fakeSent.dlc_non_comp);
  assert(!RB.canSend(0x800,&data,1));
  assert(!RB.canSend(1,nullptr,1));
  puts("PASS CAN message flags and validation");
}

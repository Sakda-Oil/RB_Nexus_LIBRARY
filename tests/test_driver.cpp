#include "RB_Nexus.h"
#include "RB_Nexus_Safety.h"
#include <driver/twai.h>
#include <cassert>
#include <cstdio>

// This test targets the production motor/PWM/CAN core; IMU is separately compiled.
bool RBNexusBoard::imuUpdate() { return false; }
float RBNexusBoard::heading() const { return NAN; }
bool RBNexusBoard::headingFresh(uint32_t) const { return false; }
uint16_t off(int channel) {
  int reg=0x06+4*channel;
  return Wire.registers[reg+2] | (Wire.registers[reg+3]<<8);
}
uint16_t on(int channel) {
  int reg=0x06+4*channel;
  return Wire.registers[reg] | (Wire.registers[reg+1]<<8);
}
int main() {
  assert(RB.begin());
  RB.motorRPM(1,100);
  fakeNow += 60; RB.update();
  assert(RB.motorPIDActive(1));
  assert(off(0)!=4096);
  RB.motorStop(1);
  fakeNow += 60; RB.update();
  assert(!RB.motorPIDActive(1));
  assert(off(0)==4096 && off(1)==4096);
  puts("PASS PID stop remains stopped after update");

  RB.motorRPM(1,100);
  RB.motorPower(1,30);
  assert(!RB.motorPIDActive(1));
  RB.motorRPM(1,0);
  assert(off(0)==4096);
  puts("PASS manual override and zero RPM");
  RB.motorPower(1,100); assert(on(0)==4096);
  RB.motorPower(1,-100); assert(on(1)==4096);
  RB.motorPower(1,32767); assert(on(0)==4096);
  RB.motorPower(1,-32768); assert(on(1)==4096);
  RB.motorPower(1,0); assert(off(0)==4096 && off(1)==4096);
  RB.motorRPM(1,100);
  RB.motorPower(1,30); assert(off(0)>1200 && off(0)<1240 && !RB.motorPIDActive(1));
  RB.motorStop(1); fakeNow+=60; RB.update(); assert(off(0)==4096 && off(1)==4096);
  puts("PASS signed motor percentage saturates safely including integer extremes");

  RB.emergencyStop();
  RB.motorPower(1,100); assert(off(0)==4096 && off(1)==4096);
  RB.forward(80); RB.motorBrake(1);
  assert(off(0)==4096 && off(1)==4096);
  RB.motorRPM(1,200); assert(!RB.motorPIDActive(1));
  RB.emergencyClear();
  RB.motorPower(1,40); assert(off(0)!=4096);
  puts("PASS emergency latch blocks every motor command");

  RBMotorCommandGuard guard(500);
  guard.feed(); fakeNow+=499; guard.update(); assert(off(0)!=4096);
  fakeNow++; guard.update(); assert(off(0)==4096);
  fakeNow=0xFFFFFF00; RB.motorPower(1,40); guard.feed();
  fakeNow+=501; guard.update(); assert(off(0)==4096);
  puts("PASS command timeout including millis rollover");

  RB.motorPower(1,40); RB.motorStop(1);
  uint32_t stopped=millis(); RB.motorPower(1,-40);
  assert(millis()-stopped>=5);
  RB.watchdogStart(100); RB.motorRPM(1,100);
  fakeNow+=101; RB.update(); assert(RB.emergencyActive());
  assert(off(0)==4096 && off(1)==4096);
  RB.watchdogStop(); RB.emergencyClear();
  puts("PASS stop/reverse dead time and watchdog");

  assert(!RB.setWheels(1,1) && !RB.setWheels(0,2) && RB.setWheels(1,2));
  RB.reverseMotor(2);
  RB.forward(50);
  assert(off(0)>2000 && off(0)<2080 && off(1)==4096); // M1 forward, about 50 %
  assert(off(2)==4096 && off(3)>2000 && off(3)<2080); // reversed M2 drives leg B
  RB.drive(0,0); assert(off(0)==4096 && off(3)==4096);
  RB.turnRight(30); assert(off(0)!=4096 && off(2)!=4096); // right wheel backwards
  RB.stop(); assert(off(0)==4096 && off(2)==4096 && off(3)==4096);
  uint32_t t0=millis(); RB.backward(40,200);
  assert(millis()-t0>=200 && off(1)==4096 && off(2)==4096); // stopped afterwards
  RB.reverseMotor(2,false);
  assert(!RB.turnDegrees(90) && off(0)==4096 && off(2)==4096); // no IMU: refuses, no motion
  assert(RB.turnDegrees(0));
  t0=millis(); RB.wait(50); assert(millis()-t0>=50);
  RB.brake(); assert(on(0)==4096 && on(1)==4096);
  RB.stop();
  puts("PASS two-wheel drive, reversed motor, timed moves, wait and IMU-less turn");

  RB.pcaFrequency(100); RB.servoPulse(1,1500);
  assert(off(8)>=600 && off(8)<=625); // servo 1 = CH8, 1500 us at actual prescaler frequency
  RB.servoRelease(1); assert(off(8)==4096);
  RB.servoPulse(8,1500); assert(off(15)>=600 && off(15)<=625);
  RB.servoPulse(0,1500); RB.servoPulse(9,1500); // ignored
  RB.pcaFrequency(50); RB.servoAngle(2,90);
  assert(off(9)>=300 && off(9)<=303); // 1472 us at 50 Hz
  puts("PASS servo numbering 1-8 and frequency");

  RB.pinPWM(4,50); assert(fakeDuty[4]==512 && fakeResolution[4]==10);
  int changes=fakeFreqChanges;
  RB.pinPWM(4,60); assert(fakeFreqChanges==changes); // same frequency: no reconfigure
  RB.pinPWM(4,60,1000); assert(fakeFreqChanges==changes+1 && fakeFreq[4]==1000);
  RB.pinPWM(4,150); assert(fakeDuty[4]==1023);
  RB.pinOff(4); assert(fakeFreq[4]==0 && fakePins[4]==LOW);
  RB.pinOn(4); assert(fakePins[4]==HIGH && RB.pinRead(4));
  RB.pinOff(4); assert(!RB.pinRead(4));
  puts("PASS GPIO on/off/read and PWM percent without repeated reconfiguration");

  fakePins[27]=HIGH;
  assert(!RB.buttonPressed(27));          // first call configures the pin
  fakePins[27]=LOW;  assert(!RB.buttonPressed(27));
  fakeNow+=10;       assert(!RB.buttonPressed(27)); // still bouncing
  fakeNow+=30;       assert(RB.buttonPressed(27));  // one press
  fakeNow+=100;      assert(!RB.buttonPressed(27)); // held: no repeat
  fakePins[27]=HIGH; RB.buttonPressed(27); fakeNow+=40; assert(!RB.buttonPressed(27));
  fakePins[27]=LOW;  RB.buttonPressed(27); fakeNow+=40; assert(RB.buttonPressed(27));
  puts("PASS debounced button press");

  assert(!RB.canBegin(500000,-1,16));
  assert(!RB.canBegin(500000,17,-1));
  assert(fakeCANInstallCount==0);
  assert(RB.canBegin());
  assert(fakeInstalledCAN.tx_io==17 && fakeInstalledCAN.rx_io==16);
  assert(fakeInstalledCAN.mode==TWAI_MODE_NORMAL);
  RB.canStop();
  assert(RB.canBegin(500000,4,5));
  assert(fakeInstalledCAN.tx_io==4 && fakeInstalledCAN.rx_io==5);
  assert(fakeCANInstallCount==2);
  puts("PASS CAN schematic default pins, explicit override and missing-pin rejection");
  uint8_t data=42;
  assert(RB.canSend(0x123,&data,1));
  assert(!fakeSent.ss && !fakeSent.self && !fakeSent.dlc_non_comp);
  assert(!RB.canSend(0x800,&data,1));
  assert(!RB.canSend(1,nullptr,1));
  puts("PASS CAN message flags and validation");
}

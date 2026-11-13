#include "RB_Nexus.h"
#include <cassert>
#include <cstdio>
#include <cmath>
bool RBNexusBoard::imuUpdate() { return false; }
float RBNexusBoard::heading() const { return NAN; }
bool RBNexusBoard::headingFresh(uint32_t) const { return false; }
void voltage(float volts) { fakeADCValue = uint16_t(lroundf(volts * 4095 / 3.3f)); }
int main() {
  assert(RB.begin());
  RBIRObstacleSensor detector(RB_PIN_D4), highDetector(RB_PIN_D14, false), invalidIR(21);
  assert(!detector.detected());
  assert(detector.begin() && highDetector.begin());
  fakePins[4] = LOW; assert(detector.detected());
  fakePins[4] = HIGH; assert(!detector.detected());
  fakePins[14] = HIGH; assert(highDetector.detected());
  assert(!invalidIR.begin() && !invalidIR.detected());
  puts("PASS digital IR polarity, initialization and reserved-pin rejection");

  RBUltrasonicSensor sonar(26,27), samePin(26,26), reserved(21,27);
  assert(isnan(sonar.cm()));
  assert(!samePin.begin() && !reserved.begin() && sonar.begin());
  fakeEchoUs = 5800; assert(fabsf(sonar.cm()-100) < .001f);
  assert(fakePulseTimeout == 30000 && sonar.status() == RBDistanceStatus::OK);
  auto calls = fakePulseCalls;
  fakeNow += 59; assert(isnan(sonar.cm()) && fakePulseCalls == calls);
  assert(sonar.status() == RBDistanceStatus::WaitForNextReading);
  ++fakeNow; assert(fabsf(sonar.cm()-100) < .001f);
  fakeNow += 60; fakeEchoUs = 0; assert(isnan(sonar.cm()));
  assert(sonar.status() == RBDistanceStatus::NoEcho);
  fakeNow += 60; fakeEchoUs = 115; assert(isnan(sonar.cm()));
  fakeNow += 60; fakeEchoUs = 116; assert(sonar.cm() == 2);
  fakeNow += 60; fakeEchoUs = 23200; assert(sonar.cm() == 400);
  fakeNow += 60; fakeEchoUs = 23201; assert(isnan(sonar.cm()));
  assert(sonar.status() == RBDistanceStatus::OutOfRange);
  sonar.begin(); fakeNow = 0xFFFFFFE0u; fakeEchoUs = 580;
  assert(sonar.cm() == 10);
  fakeNow += 60; assert(sonar.cm() == 10);
  puts("PASS ultrasonic cm units, bounded timeout, range, rate limit and millis rollover");

  RBIRDistanceSensor ir(1), badChannel(0), custom(8,RBIRModel::Custom);
  assert(isnan(ir.cm()));
  assert(!badChannel.begin() && isnan(badChannel.volts()));
  assert(ir.begin() && custom.begin());
  voltage(1.3f); assert(fabsf(ir.cm()-20) < .03f);
  voltage(0); assert(isnan(ir.cm()));
  voltage(3.3f); assert(isnan(ir.cm()));
  assert(ir.status() == RBDistanceStatus::OutOfRange);
  voltage(1.5f); assert(isnan(custom.cm()));
  assert(custom.status() == RBDistanceStatus::InvalidCalibration);
  assert(fabsf(custom.volts()-1.5f) < .001f);
  RBIRCalibrationPoint points[] = {{10,2},{20,1}};
  assert(custom.setCalibration(points,2));
  points[0].volts = 3; // the library owns its copy
  assert(fabsf(custom.cm()-13.33333f) < .02f);
  const RBIRCalibrationPoint reversed[] = {{20,1},{10,2}};
  const RBIRCalibrationPoint nan[] = {{10,2},{NAN,1}};
  assert(!custom.setCalibration(reversed,2) && !custom.setCalibration(nan,2));
  assert(!custom.setCalibration(nullptr,2) && !custom.setCalibration(points,1));
  assert(fabsf(custom.cm()-13.33333f) < .02f);
  voltage(0.9f); assert(isnan(custom.cm()));
  puts("PASS analog IR conversion, copied calibration, invalid tables and unknown distance");
}

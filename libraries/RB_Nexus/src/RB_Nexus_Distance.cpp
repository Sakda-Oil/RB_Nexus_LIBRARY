// SPDX-License-Identifier: LGPL-2.1-or-later
#include "RB_Nexus.h"
#include <math.h>

namespace {
bool sensorPin(uint8_t pin) {
  return pin == RB_PIN_D4 || pin == RB_PIN_D12 || pin == RB_PIN_D14 ||
         pin == RB_PIN_D26 || pin == RB_PIN_D27;
}
}

const char* distanceStatusText(RBDistanceStatus status) {
  switch (status) {
    case RBDistanceStatus::OK: return "OK";
    case RBDistanceStatus::WaitForNextReading: return "Wait at least 60 ms between pings";
    case RBDistanceStatus::NoEcho: return "No echo: check wiring or target";
    case RBDistanceStatus::OutOfRange: return "Outside measurement/calibration range";
    case RBDistanceStatus::InvalidCalibration: return "Invalid or missing calibration";
    default: return "Call begin() and check its result first";
  }
}

bool RBIRObstacleSensor::begin() {
  _ready = sensorPin(_pin);
  if (_ready) ::pinMode(_pin, INPUT);
  return _ready;
}
bool RBIRObstacleSensor::isObstacleDetected() const {
  return _ready && (::digitalRead(_pin) == (_activeLow ? LOW : HIGH));
}

bool RBUltrasonicSensor::begin() {
  _ready = sensorPin(_trigger) && sensorPin(_echo) && _trigger != _echo;
  _hasPing = false;
  _status = RBDistanceStatus::NotInitialized;
  if (_ready) {
    ::digitalWrite(_trigger, LOW);
    ::pinMode(_trigger, OUTPUT);
    ::pinMode(_echo, INPUT);
  }
  return _ready;
}
float RBUltrasonicSensor::readCentimeters() {
  if (!_ready) { _status = RBDistanceStatus::NotInitialized; return NAN; }
  const uint32_t now = millis();
  if (_hasPing && uint32_t(now - _lastPing) < 60) {
    _status = RBDistanceStatus::WaitForNextReading;
    return NAN;
  }
  _lastPing = now; _hasPing = true;
  ::digitalWrite(_trigger, LOW); delayMicroseconds(2);
  ::digitalWrite(_trigger, HIGH); delayMicroseconds(10);
  ::digitalWrite(_trigger, LOW);
  const uint32_t duration = pulseIn(_echo, HIGH, 30000UL);
  if (!duration) { _status = RBDistanceStatus::NoEcho; return NAN; }
  const float cm = duration / 58.0f;
  if (cm < 2.0f || cm > 400.0f) { _status = RBDistanceStatus::OutOfRange; return NAN; }
  _status = RBDistanceStatus::OK;
  return cm;
}

RBIRDistanceSensor::RBIRDistanceSensor(uint8_t channel, RBIRModel model) : _channel(channel) {
  if (model == RBIRModel::Sharp10To80CM) {
    // Approximate typical curve, Sharp GP2Y0A21YK0F datasheet Fig.2, page 5.
    // These are starting values, not guaranteed sensor accuracy.
    const RBIRCalibrationPoint defaults[] = {
      {10,2.30f}, {15,1.65f}, {20,1.30f}, {30,0.92f},
      {40,0.72f}, {50,0.60f}, {60,0.50f}, {80,0.40f}
    };
    setCalibration(defaults, sizeof(defaults)/sizeof(defaults[0]));
  }
}
bool RBIRDistanceSensor::setCalibration(const RBIRCalibrationPoint* points, size_t count) {
  if (!points || count < 2 || count > 12) return false;
  for (size_t i = 0; i < count; ++i) {
    if (!isfinite(points[i].centimeters) || points[i].centimeters <= 0 ||
        !isfinite(points[i].volts) || points[i].volts <= 0 || points[i].volts > 3.3f ||
        (i && (points[i].centimeters <= points[i-1].centimeters ||
               points[i].volts >= points[i-1].volts))) return false;
  }
  for (size_t i = 0; i < count; ++i) _points[i] = points[i];
  _count = count;
  return true;
}
bool RBIRDistanceSensor::begin() {
  _ready = _channel >= RB_ADC_CH_MIN && _channel <= RB_ADC_CH_MAX;
  _status = _ready && _count < 2 ? RBDistanceStatus::InvalidCalibration : RBDistanceStatus::NotInitialized;
  // Voltage can be read for calibration even for a Custom sensor without a table.
  return _ready;
}
float RBIRDistanceSensor::readVoltage() {
  if (!_ready) { _status = RBDistanceStatus::NotInitialized; return NAN; }
  _status = RBDistanceStatus::OK;
  return RB.analogReadAverage(_channel, 8) * (3.3f / 4095.0f);
}
float RBIRDistanceSensor::readCentimeters() {
  const float voltage = readVoltage();
  if (!isfinite(voltage)) return NAN;
  if (_count < 2) { _status = RBDistanceStatus::InvalidCalibration; return NAN; }
  if (voltage > _points[0].volts || voltage < _points[_count-1].volts) {
    _status = RBDistanceStatus::OutOfRange; return NAN;
  }
  for (size_t i = 1; i < _count; ++i) {
    if (voltage >= _points[i].volts) {
      const auto& near = _points[i-1]; const auto& far = _points[i];
      const float fraction = (near.volts - voltage) / (near.volts - far.volts);
      // Inverse distance is approximately linear with Sharp output voltage.
      return 1.0f / ((1.0f-fraction)/near.centimeters + fraction/far.centimeters);
    }
  }
  _status = RBDistanceStatus::OutOfRange;
  return NAN;
}

// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <Arduino.h>

// Digital obstacle detectors return a boolean, never a distance.
class RBIRObstacleSensor {
public:
  explicit RBIRObstacleSensor(uint8_t pin, bool activeLow = true)
      : _pin(pin), _activeLow(activeLow) {}
  bool begin();
  bool isReady() const { return _ready; }
  bool isObstacleDetected() const;
  bool detected() const { return isObstacleDetected(); }
private:
  uint8_t _pin;
  bool _activeLow, _ready = false;
};

enum class RBDistanceStatus : uint8_t {
  NotInitialized, OK, WaitForNextReading, NoEcho, OutOfRange, InvalidCalibration
};
const char* distanceStatusText(RBDistanceStatus status);

// HC-SR04: GPIO echo must be level shifted to <=3.3 V. Measure at >=60 ms intervals.
class RBUltrasonicSensor {
public:
  RBUltrasonicSensor(uint8_t triggerPin, uint8_t echoPin)
      : _trigger(triggerPin), _echo(echoPin) {}
  bool begin();
  bool isReady() const { return _ready; }
  // Returns NAN on failure; blocks for at most approximately 30 ms per ping.
  float readCentimeters();
  float cm() { return readCentimeters(); }
  RBDistanceStatus status() const { return _status; }
  const char* statusText() const { return distanceStatusText(_status); }
private:
  uint8_t _trigger, _echo;
  bool _ready = false, _hasPing = false;
  uint32_t _lastPing = 0;
  RBDistanceStatus _status = RBDistanceStatus::NotInitialized;
};

enum class RBIRModel : uint8_t { Sharp10To80CM, Custom };
struct RBIRCalibrationPoint { float centimeters; float volts; };

// Analog connector A1..A8 (MCP3208), not an ESP32 GPIO number.
class RBIRDistanceSensor {
public:
  explicit RBIRDistanceSensor(uint8_t analogChannel, RBIRModel model = RBIRModel::Sharp10To80CM);
  bool begin(); // Call RB.begin() first to initialize the ADC bus.
  bool isReady() const { return _ready; }
  float readVoltage();
  float volts() { return readVoltage(); }
  // Approximate Sharp GP2Y0A21YK0F curve; calibrate against a ruler for your sensor.
  float readCentimeters();
  float cm() { return readCentimeters(); }
  // Copies 2..12 points: distance increases, voltage decreases, voltage <=3.3 V.
  // Failed changes leave the last valid calibration intact.
  bool setCalibration(const RBIRCalibrationPoint* points, size_t count);
  RBDistanceStatus status() const { return _status; }
  const char* statusText() const { return distanceStatusText(_status); }
private:
  uint8_t _channel;
  bool _ready = false;
  RBIRCalibrationPoint _points[12] = {};
  size_t _count = 0;
  RBDistanceStatus _status = RBDistanceStatus::NotInitialized;
};

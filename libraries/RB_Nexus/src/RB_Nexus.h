// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file RB_Nexus.h
 * @brief Core Arduino library for Redbrick Robotics RB_Nexus ESP32 Controller.
 *
 * Every command has exactly one name. Numbering always starts at 1
 * (motor 1-4, servo 1-8, analog 1-8, encoder 1-4) and power is always
 * a percentage (-100..100).
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "RB_Nexus_Pins.h"
#include "RB_Nexus_Distance.h"

#if !defined(ARDUINO_ARCH_ESP32)
#error "RB_Nexus requires an ESP32 board selected in Tools > Board."
#endif

namespace RBNexus {
constexpr const char* name = "RB_Nexus";
constexpr const char* version = "0.3.0";
constexpr bool peripheralPinMapAvailable = true;
}

enum class RBIMUType : uint8_t { Auto, MPU9250, BNO085, MPU6050, MPU6500 };
struct sh2_SensorEvent;

// PID Controller state (one per motor)
struct RBPIDConfig {
  float kp = 1.2f;
  float ki = 0.5f;
  float kd = 0.05f;
  float targetRPM = 0.0f;
  float integral = 0.0f;
  float prevError = 0.0f;
  unsigned long lastTime = 0;
  bool enabled = false;
  bool primed = false;
};

// =============================================================================
// RBNexusBoard Driver Class
// =============================================================================
class RBNexusBoard {
public:
  RBNexusBoard();

  // --- System ---
  bool begin(uint32_t i2cFreq = 400000, uint32_t spiFreq = 1000000);
  void update();               // Call every loop(): PID, encoder RPM, IMU, watchdog
  void wait(uint32_t ms);      // Like delay(), but keeps update() running

  // --- Status LED ---
  void led(bool on);
  void ledToggle();

  // --- Two-wheel driving (power: 0..100 %) ---
  // Pick the motors once in setup(); defaults are left = M1, right = M2.
  bool setWheels(uint8_t leftMotor, uint8_t rightMotor);
  void reverseMotor(uint8_t motorId, bool reversed = true); // Wired backwards
  // durationMs = 0 keeps moving; > 0 moves for that long, then stops.
  void forward(int16_t power, uint32_t durationMs = 0);
  void backward(int16_t power, uint32_t durationMs = 0);
  void turnLeft(int16_t power, uint32_t durationMs = 0);   // Spin in place
  void turnRight(int16_t power, uint32_t durationMs = 0);  // Spin in place
  void drive(int16_t leftPower, int16_t rightPower);       // -100..100 each
  // Turn using the IMU heading: + = right, - = left. Needs imuBegin().
  bool turnDegrees(float degrees, int16_t power = 40, uint32_t timeoutMs = 5000);
  void stop();   // Stop all motors (coast)
  void brake();  // Brake all motors

  // --- Single motor M1-M4 ---
  void motorPower(uint8_t motorId, int16_t percent); // -100..100
  void motorRPM(uint8_t motorId, float rpm);         // Closed loop, needs encoder
  void motorStop(uint8_t motorId);
  void motorBrake(uint8_t motorId);
  void motorPID(uint8_t motorId, float kp, float ki, float kd);
  bool motorPIDActive(uint8_t motorId) const;
  void motorDeadTime(uint16_t ms) { _deadTimeMs = ms; } // Pause when reversing

  // --- Servo 1-8 (PCA9685 channels 8-15) ---
  void servoAngle(uint8_t servo, uint8_t degrees, uint16_t minUs = 544, uint16_t maxUs = 2400);
  void servoPulse(uint8_t servo, uint16_t microseconds); // 500..2500 us
  void servoRelease(uint8_t servo);

  // --- Encoder 1-4 ---
  void    encoderBegin();
  int32_t encoderTicks(uint8_t encId);
  float   encoderRPM(uint8_t encId);   // Signed: + forward, - reverse
  void    encoderReset(uint8_t encId);
  void    encoderResetAll();
  void    encoderSetPPR(uint8_t encId, uint16_t ticksPerRev);

  // --- Analog A1-A8 (MCP3208 12-bit) ---
  uint16_t analogRaw(uint8_t channel);                       // 0..4095
  float    analogVolt(uint8_t channel, float vref = 3.3f);   // 0..3.3 V
  float    analogPercent(uint8_t channel);                   // 0..100 %
  uint16_t analogAverage(uint8_t channel, uint16_t samples = 10);

  // --- ESP32 GPIO pins (e.g. 4, 12, 14, 26, 27) ---
  void pinOn(uint8_t pin);           // OUTPUT HIGH
  void pinOff(uint8_t pin);          // OUTPUT LOW (also stops PWM)
  bool pinRead(uint8_t pin);         // true = HIGH
  bool pinPWM(uint8_t pin, float percent, uint32_t frequencyHz = 5000);
  // Button between pin and GND: true once per press (debounced, internal pull-up).
  bool buttonPressed(uint8_t pin);

  // --- PCA9685 raw channels 0-15 (advanced) ---
  bool pcaReady() const { return _pcaConnected; }
  void pcaDuty(uint8_t channel, uint16_t duty); // 0..4095
  void pcaFrequency(float hz); // Shared by motors AND servos: servos need 50 Hz

  // --- IMU (I2C): MPU9250/MPU6050/MPU6500 0x68/0x69; BNO085 0x4A/0x4B ---
  bool  imuBegin(RBIMUType type = RBIMUType::Auto, uint8_t address = 0);
  bool  imuUpdate(); // Advanced: update() already calls this. true = new report.
  bool  imuReady() const { return _imuDetected; }
  bool  imuFresh(uint32_t maxAgeMs = 500) const;            // Accel + gyro
  bool  imuOrientationFresh(uint32_t maxAgeMs = 500) const; // Roll/pitch/yaw
  RBIMUType imuType() const { return _imuType; }
  const char* imuName() const { return _imuModel; }
  uint32_t imuSampleCount() const { return _imuGyroSamples; }
  // Heading: accumulated gyro degrees, right/clockwise positive with +Z up.
  float heading() const;
  bool  headingFresh(uint32_t maxAgeMs = 500) const;
  bool  resetHeading(float degrees = 0.0f);
  bool  setHeadingBias(float radiansPerSecond); // Measure while stationary
  // Acceleration m/s^2 (incl. gravity), gyro rad/s, magnetic field uT.
  float accelX();
  float accelY();
  float accelZ();
  float gyroX();
  float gyroY();
  float gyroZ();
  float magX();
  float magY();
  float magZ();
  // Euler angles in degrees, native sensor/fusion frame.
  float roll();
  float pitch();
  float yaw();
  float quaternionW() const { return _quaternion[0]; }
  float quaternionX() const { return _quaternion[1]; }
  float quaternionY() const { return _quaternion[2]; }
  float quaternionZ() const { return _quaternion[3]; }

  // --- CAN Bus (TWAI): board TX=GPIO17, RX=GPIO16 ---
  bool canBegin(uint32_t baudRate = 500000, int txPin = RB_PIN_CAN_TX, int rxPin = RB_PIN_CAN_RX);
  bool canSend(uint32_t id, const uint8_t* data, uint8_t len, bool ext = false);
  bool canReceive(uint32_t& id, uint8_t* data, uint8_t& len, bool& ext);
  void canStop();

  // --- Wi-Fi ---
  bool   wifiConnect(const char* ssid, const char* pass, uint32_t timeoutMs = 10000);
  bool   wifiConnected();
  int8_t wifiRSSI();
  String wifiIP();
  void   wifiAutoReconnect(bool enable = true) { _wifiAutoReconnect = enable; }

  // --- I2C ---
  int i2cScan(Print* out = &Serial);

  // --- Safety ---
  void emergencyStop();                 // Stops motors and latches
  void emergencyClear() { _emergencyStopActive = false; }
  bool emergencyActive() const { return _emergencyStopActive; }
  void watchdogStart(uint32_t timeoutMs = 2000); // Stop if not fed in time
  void watchdogStop() { _watchdogEnabled = false; }
  void watchdogFeed();

private:
  struct ButtonState {
    bool ready = false, stable = true, last = true;
    uint32_t changedAt = 0;
  };

  bool _pcaConnected;
  uint32_t _spiFreq;
  bool _encodersInitialized;
  bool _emergencyStopActive;
  uint16_t _deadTimeMs;

  // Motor state tracking for reversal dead-time
  int8_t _motorLastDir[4]; // +1, -1, 0
  unsigned long _motorLastStopTime[4];
  bool _motorReversed[4] = {};
  uint8_t _leftMotor = 1, _rightMotor = 2;

  // Encoder speed calculation state
  int32_t  _lastEncTicks[4];
  unsigned long _lastEncTime[4];
  float    _currentRPM[4];
  uint16_t _encPPR[4];

  // PID controllers for 4 motors
  RBPIDConfig _pid[4];

  // IMU state
  bool _imuDetected;
  const char* _imuModel;
  uint8_t _imuAddress;
  float _accel[3];
  float _gyro[3];
  float _angles[3]; // Roll, Pitch, Yaw

  // CAN (TWAI) state
  bool _canInitialized;

  // Wi-Fi state
  bool _wifiAutoReconnect;
  unsigned long _lastWiFiCheck;

  // Watchdog
  bool _watchdogEnabled;
  uint32_t _watchdogTimeoutMs;
  unsigned long _lastWatchdogFeed;

  float _pwmFrequency = 50.0f;
  uint32_t _lastRPMUpdate = 0;
  uint32_t _gpioPWMFreq[40] = {}; // 0 = no PWM attached to that GPIO
  ButtonState _buttons[40];
  RBIMUType _imuType = RBIMUType::Auto;
  uint8_t _imuReports = 0;
  uint32_t _imuAccelTime = 0, _imuGyroTime = 0, _imuOrientationTime = 0;
  float _mag[3] = {};
  float _quaternion[4] = {1, 0, 0, 0};
  float _rotationZ = 0, _gyroZBias = 0, _previousGyroZ = 0;
  uint32_t _rotationZSampleUs = 0, _rotationZReceiveMs = 0;
  bool _rotationZPrimed = false;
  uint32_t _imuGyroSamples = 0, _bnoReportCount = 0;
  void updateRotationZ(float radiansPerSecond, uint32_t sampleUs);
  void receiveBNOEvent(sh2_SensorEvent* event);
  bool recoverBNOReset();

  uint8_t readRegister8(uint8_t reg);
  void writeRegister8(uint8_t reg, uint8_t value);
  void pcaWrite(uint8_t channel, uint16_t on, uint16_t off);
  void servoChannelPulse(uint8_t channel, uint16_t us);
  void releasePWM(uint8_t pin);
  void motorSpeed255(uint8_t motorId, int16_t speed); // Manual -255..255, disables PID
  void updatePID();
  void updateEncodersRPM();
  void motorOutputStop(uint8_t motorId);
  void motorOutputSet(uint8_t motorId, int16_t speed);
};

// Global board instance
extern RBNexusBoard RB;

// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file RB_Nexus.h
 * @brief Core Arduino library for Redbrick Robotics RB_Nexus ESP32 Controller.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "RB_Nexus_Pins.h"

#if !defined(ARDUINO_ARCH_ESP32)
#error "RB_Nexus requires an ESP32 board selected in Tools > Board."
#endif

namespace RBNexus {
constexpr const char* name = "RB_Nexus";
constexpr const char* version = "0.2.0";
constexpr bool peripheralPinMapAvailable = true;
}

// PID Controller configuration structure
struct RBPIDConfig {
  float kp = 1.2f;
  float ki = 0.5f;
  float kd = 0.05f;
  float targetRPM = 0.0f;
  float currentRPM = 0.0f;
  float integral = 0.0f;
  float prevError = 0.0f;
  unsigned long lastTime = 0;
  bool enabled = false;
};

// =============================================================================
// RBNexusBoard Driver Class
// =============================================================================
class RBNexusBoard {
public:
  RBNexusBoard();

  // --- System Initialization & Update ---
  bool begin(uint32_t i2cFreq = 400000, uint32_t spiFreq = 1000000);
  void update(); // Call in loop() for PID, encoder RPM calculations, safety watchdog

  // --- Status LED ---
  void setLED(bool on);
  void toggleLED();

  // --- Digital I/O Helpers ---
  int  digitalRead(uint8_t pin);
  void digitalWrite(uint8_t pin, uint8_t val);
  void pinMode(uint8_t pin, uint8_t mode);

  // --- ESP32 Internal LEDC PWM ---
  bool pwmWrite(uint8_t pin, uint32_t duty, uint32_t freq = 5000, uint8_t resolution = 8);
  bool pwmSetFrequency(uint8_t pin, uint32_t hz);
  void pwmStop(uint8_t pin);

  // --- MCP3208 12-bit SPI ADC (Channels 1 to 8) ---
  uint16_t analogRead(uint8_t channel);
  float    analogVoltage(uint8_t channel, float vref = 3.3f);
  uint16_t analogReadAverage(uint8_t channel, uint16_t samples = 10);
  uint16_t analogReadMin(uint8_t channel, uint16_t samples = 10);
  uint16_t analogReadMax(uint8_t channel, uint16_t samples = 10);

  // Compatibility alias
  uint16_t readAnalog(uint8_t channel) { return analogRead(channel); }
  float    readAnalogVoltage(uint8_t channel, float vref = 3.3f) { return analogVoltage(channel, vref); }

  // --- PCA9685 PWM Controller (Motors & Servos) ---
  bool isPCA9685Connected() const { return _pcaConnected; }
  void setPWM(uint8_t channel, uint16_t on, uint16_t off);
  void setPWMDuty(uint8_t channel, uint16_t duty); // 0 - 4095
  void setPWMFreq(float freqHz);

  // --- DC Motor Control (Channels 1 to 4) ---
  // speed: -255 to +255 (Standard Robotics Scale)
  void motorSet(uint8_t motorId, int16_t speed);
  void motorForward(uint8_t motorId, uint8_t speed = 200);
  void motorBackward(uint8_t motorId, uint8_t speed = 200);
  void motorStop(uint8_t motorId);
  void motorBrake(uint8_t motorId);
  void motorStopAll();
  void emergencyStop();
  bool isEmergencyStopped() const { return _emergencyStopActive; }
  void clearEmergencyStop() { _emergencyStopActive = false; }
  void setMotorDeadTimeMs(uint16_t ms) { _deadTimeMs = ms; }

  // Backward-compatible alias
  void motor(uint8_t motorId, int16_t speed) { motorSet(motorId, speed); }

  // --- Motor Closed-Loop Speed Control (PID) ---
  void motorSetRPM(uint8_t motorId, float rpm);
  void motorSetPID(uint8_t motorId, float kp, float ki, float kd);
  void motorPIDEnable(uint8_t motorId);
  void motorPIDDisable(uint8_t motorId);
  bool isMotorPIDEnabled(uint8_t motorId) const;

  // --- Servo Control (PCA9685 Channels 8 to 15) ---
  void servoAttach(uint8_t channel);
  void servoWrite(uint8_t channel, uint8_t angle, uint16_t minUs = 544, uint16_t maxUs = 2400);
  void servoWriteMicroseconds(uint8_t channel, uint16_t us);
  void servoDetach(uint8_t channel);

  // Backward-compatible alias
  void servo(uint8_t channel, uint8_t angle, uint16_t minUs = 544, uint16_t maxUs = 2400) {
    servoWrite(channel, angle, minUs, maxUs);
  }
  void servoMicroseconds(uint8_t channel, uint16_t us) {
    servoWriteMicroseconds(channel, us);
  }

  // --- Quadrature Encoders (Channels 1 to 4) ---
  void    encoderBegin();
  int32_t encoderRead(uint8_t encId);
  void    encoderReset(uint8_t encId);
  void    encoderResetAll();
  float   encoderRPM(uint8_t encId);
  int8_t  encoderDirection(uint8_t encId); // +1: Forward, -1: Reverse, 0: Stopped
  void    encoderSetPPR(uint8_t encId, uint16_t ppr);

  // Backward-compatible alias
  int32_t readEncoder(uint8_t encId) { return encoderRead(encId); }
  void    resetEncoder(uint8_t encId) { encoderReset(encId); }
  void    resetAllEncoders() { encoderResetAll(); }

  // --- I2C Bus Utilities ---
  bool i2cBegin(uint32_t freq = 400000);
  int  i2cScan(Print* out = &Serial);

  // --- IMU (Modular / Auto-probe) ---
  // Status: UNVERIFIED on V0.1 PCB (Pending schematic)
  bool  imuBegin();
  bool  isIMUAvailable() const { return _imuDetected; }
  const char* imuModelName() const { return _imuModel; }
  float accelX();
  float accelY();
  float accelZ();
  float gyroX();
  float gyroY();
  float gyroZ();
  float roll();
  float pitch();
  float yaw();

  // --- CAN Bus (TWAI) ---
  // Status: COMPILE VERIFIED / HARDWARE TRANSCEIVER PINS UNVERIFIED
  bool canBegin(uint32_t baudRate = 500000, int txPin = RB_PIN_CAN_TX, int rxPin = RB_PIN_CAN_RX);
  bool canSend(uint32_t id, const uint8_t* data, uint8_t len, bool ext = false);
  bool canReceive(uint32_t& id, uint8_t* data, uint8_t& len, bool& ext);
  void canStop();

  // --- Wi-Fi Utilities ---
  bool        wifiConnected();
  int8_t      wifiRSSI();
  String      wifiIP();
  bool        wifiConnect(const char* ssid, const char* pass, uint32_t timeoutMs = 10000);
  void        wifiEnableAutoReconnect(bool enable = true) { _wifiAutoReconnect = enable; }

  // --- Safety Watchdog ---
  void enableWatchdog(uint32_t timeoutMs = 2000);
  void disableWatchdog() { _watchdogEnabled = false; }
  void feedWatchdog();

private:
  bool _pcaConnected;
  uint32_t _spiFreq;
  bool _encodersInitialized;
  bool _emergencyStopActive;
  uint16_t _deadTimeMs;

  // Motor state tracking for reversal dead-time
  int8_t _motorLastDir[4]; // +1, -1, 0
  unsigned long _motorLastStopTime[4];

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

  uint8_t readRegister8(uint8_t reg);
  void writeRegister8(uint8_t reg, uint8_t value);
  void updatePID();
  void updateEncodersRPM();
};

// Global singleton instance
extern RBNexusBoard RB;
extern RBNexusBoard RB_Nexus;

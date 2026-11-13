// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file RB_Nexus.cpp
 * @brief Implementation of RB_Nexus ESP32 Robotics Controller Library.
 */

#include "RB_Nexus.h"
#include <WiFi.h>
#include <math.h>
#include <driver/gpio.h>
#include <driver/twai.h>

// PCA9685 Registers
static constexpr uint8_t PCA9685_MODE1     = 0x00;
static constexpr uint8_t PCA9685_MODE2     = 0x01;
static constexpr uint8_t PCA9685_LED0_ON_L = 0x06;
static constexpr uint8_t PCA9685_PRESCALE  = 0xFE;

static constexpr uint8_t GPIO_PWM_BITS = 10;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;

static inline bool validMotor(uint8_t id) { return id >= 1 && id <= 4; }
static inline bool validServo(uint8_t n) { return n >= 1 && n <= RB_SERVO_COUNT; }
static inline int16_t clampPercent(int32_t p) { return p > 100 ? 100 : (p < -100 ? -100 : p); }

// Encoder ISR state
static volatile int32_t s_encTicks[4] = {0, 0, 0, 0};

static void IRAM_ATTR isrEnc1A() {
  bool a = digitalRead(RB_PIN_ENC1_A);
  bool b = digitalRead(RB_PIN_ENC1_B);
  s_encTicks[0] += (a == b) ? 1 : -1;
}

static void IRAM_ATTR isrEnc2A() {
  bool a = digitalRead(RB_PIN_ENC2_A);
  bool b = digitalRead(RB_PIN_ENC2_B);
  s_encTicks[1] += (a == b) ? 1 : -1;
}

static void IRAM_ATTR isrEnc3A() {
  bool a = digitalRead(RB_PIN_ENC3_A);
  bool b = digitalRead(RB_PIN_ENC3_B);
  s_encTicks[2] += (a == b) ? 1 : -1;
}

static void IRAM_ATTR isrEnc4A() {
  bool a = digitalRead(RB_PIN_ENC4_A);
  bool b = digitalRead(RB_PIN_ENC4_B);
  s_encTicks[3] += (a == b) ? 1 : -1;
}

RBNexusBoard::RBNexusBoard()
  : _pcaConnected(false),
    _spiFreq(1000000),
    _encodersInitialized(false),
    _emergencyStopActive(false),
    _deadTimeMs(5),
    _imuDetected(false),
    _imuModel("None"),
    _imuAddress(0),
    _canInitialized(false),
    _wifiAutoReconnect(false),
    _lastWiFiCheck(0),
    _watchdogEnabled(false),
    _watchdogTimeoutMs(2000),
    _lastWatchdogFeed(0) {

  for (int i = 0; i < 4; ++i) {
    _motorLastDir[i] = 0;
    _motorLastStopTime[i] = 0;
    _lastEncTicks[i] = 0;
    _lastEncTime[i] = 0;
    _currentRPM[i] = 0.0f;
    _encPPR[i] = 330; // Default: 11 poles * 30:1 gear ratio = 330 ticks/rev
  }

  for (int i = 0; i < 3; ++i) {
    _accel[i] = 0.0f;
    _gyro[i] = 0.0f;
    _angles[i] = 0.0f;
  }
}

// =============================================================================
// System
// =============================================================================
bool RBNexusBoard::begin(uint32_t i2cFreq, uint32_t spiFreq) {
  _spiFreq = spiFreq;

  ::pinMode(RB_PIN_LED, OUTPUT);
  ::digitalWrite(RB_PIN_LED, LOW);

  // I2C bus for PCA9685 (and IMU)
  Wire.begin(RB_PIN_I2C_SDA, RB_PIN_I2C_SCL, i2cFreq);

  Wire.beginTransmission(RB_PCA9685_ADDR);
  _pcaConnected = (Wire.endTransmission() == 0);

  if (_pcaConnected) {
    writeRegister8(PCA9685_MODE1, 0x20); // Auto-increment enabled
    writeRegister8(PCA9685_MODE2, 0x04); // Totem-pole output
    pcaFrequency(50.0f);                 // 50 Hz default for servos and motors
    for (uint8_t ch = 0; ch < 16; ++ch) {
      pcaDuty(ch, 0);
    }
  }

  // SPI bus for MCP3208
  ::pinMode(RB_PIN_SPI_CS, OUTPUT);
  ::digitalWrite(RB_PIN_SPI_CS, HIGH);
  SPI.begin(RB_PIN_SPI_CLK, RB_PIN_SPI_MISO, RB_PIN_SPI_MOSI, RB_PIN_SPI_CS);

  watchdogFeed();
  return _pcaConnected;
}

void RBNexusBoard::update() {
  unsigned long now = millis();

  if (_watchdogEnabled && (now - _lastWatchdogFeed > _watchdogTimeoutMs)) {
    if (!_emergencyStopActive) {
      emergencyStop();
    }
  }

  // Encoder RPM and PID (every 50 ms)
  if (now - _lastRPMUpdate >= 50) {
    _lastRPMUpdate = now;
    updateEncodersRPM();
    updatePID();
  }

  imuUpdate();

  // Background Wi-Fi auto-reconnect check (every 5 seconds)
  if (_wifiAutoReconnect && (now - _lastWiFiCheck >= 5000)) {
    _lastWiFiCheck = now;
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_OFF) {
      WiFi.reconnect();
    }
  }
}

void RBNexusBoard::wait(uint32_t ms) {
  const uint32_t start = millis();
  while (uint32_t(millis() - start) < ms) {
    update();
    delay(1);
  }
}

// =============================================================================
// Status LED
// =============================================================================
void RBNexusBoard::led(bool on) {
  ::digitalWrite(RB_PIN_LED, on ? HIGH : LOW);
}

void RBNexusBoard::ledToggle() {
  ::digitalWrite(RB_PIN_LED, !::digitalRead(RB_PIN_LED));
}

// =============================================================================
// Two-wheel driving
// =============================================================================
bool RBNexusBoard::setWheels(uint8_t leftMotor, uint8_t rightMotor) {
  if (!validMotor(leftMotor) || !validMotor(rightMotor) || leftMotor == rightMotor) return false;
  _leftMotor = leftMotor;
  _rightMotor = rightMotor;
  return true;
}

void RBNexusBoard::reverseMotor(uint8_t motorId, bool reversed) {
  if (validMotor(motorId)) _motorReversed[motorId - 1] = reversed;
}

void RBNexusBoard::drive(int16_t leftPower, int16_t rightPower) {
  motorPower(_leftMotor, leftPower);
  motorPower(_rightMotor, rightPower);
}

void RBNexusBoard::forward(int16_t power, uint32_t durationMs) {
  const int16_t p = abs(clampPercent(power));
  drive(p, p);
  if (durationMs) { wait(durationMs); stop(); }
}

void RBNexusBoard::backward(int16_t power, uint32_t durationMs) {
  const int16_t p = abs(clampPercent(power));
  drive(-p, -p);
  if (durationMs) { wait(durationMs); stop(); }
}

void RBNexusBoard::turnLeft(int16_t power, uint32_t durationMs) {
  const int16_t p = abs(clampPercent(power));
  drive(-p, p);
  if (durationMs) { wait(durationMs); stop(); }
}

void RBNexusBoard::turnRight(int16_t power, uint32_t durationMs) {
  const int16_t p = abs(clampPercent(power));
  drive(p, -p);
  if (durationMs) { wait(durationMs); stop(); }
}

bool RBNexusBoard::turnDegrees(float degrees, int16_t power, uint32_t timeoutMs) {
  if (!isfinite(degrees)) return false;
  if (degrees == 0.0f) return true;
  if (_emergencyStopActive || !headingFresh()) return false;

  const float target = heading() + degrees;
  const int16_t p = abs(clampPercent(power));
  if (degrees > 0) drive(p, -p); else drive(-p, p);

  bool reached = false;
  const uint32_t start = millis();
  while (uint32_t(millis() - start) < timeoutMs) {
    update();
    if (_emergencyStopActive || !headingFresh(200)) break;
    const float h = heading();
    if ((degrees > 0 && h >= target) || (degrees < 0 && h <= target)) {
      reached = true;
      break;
    }
    delay(1);
  }
  stop();
  return reached;
}

void RBNexusBoard::stop() {
  for (uint8_t id = 1; id <= 4; ++id) motorStop(id);
}

void RBNexusBoard::brake() {
  for (uint8_t id = 1; id <= 4; ++id) motorBrake(id);
}

// =============================================================================
// ESP32 GPIO pins
// =============================================================================
void RBNexusBoard::releasePWM(uint8_t pin) {
  if (pin < 40 && _gpioPWMFreq[pin]) {
    ledcDetach(pin);
    _gpioPWMFreq[pin] = 0;
  }
}

void RBNexusBoard::pinOn(uint8_t pin) {
  if (pin >= 40) return;
  releasePWM(pin);
  _buttons[pin].ready = false;
  ::pinMode(pin, OUTPUT);
  ::digitalWrite(pin, HIGH);
}

void RBNexusBoard::pinOff(uint8_t pin) {
  if (pin >= 40) return;
  releasePWM(pin);
  _buttons[pin].ready = false;
  ::pinMode(pin, OUTPUT);
  ::digitalWrite(pin, LOW);
}

bool RBNexusBoard::pinRead(uint8_t pin) {
  return pin < 40 && ::digitalRead(pin) == HIGH;
}

bool RBNexusBoard::pinPWM(uint8_t pin, float percent, uint32_t frequencyHz) {
  if (pin >= 40 || frequencyHz == 0 || !isfinite(percent)) return false;
  // Compare against the requested frequency: the hardware may round the real one.
  if (_gpioPWMFreq[pin] == 0) {
    if (!ledcAttach(pin, frequencyHz, GPIO_PWM_BITS)) return false;
  } else if (_gpioPWMFreq[pin] != frequencyHz) {
    if (ledcChangeFrequency(pin, frequencyHz, GPIO_PWM_BITS) == 0) return false;
  }
  _gpioPWMFreq[pin] = frequencyHz;
  _buttons[pin].ready = false;
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  const uint32_t maximum = (1UL << GPIO_PWM_BITS) - 1;
  return ledcWrite(pin, uint32_t(lroundf(percent * maximum / 100.0f)));
}

bool RBNexusBoard::buttonPressed(uint8_t pin) {
  if (pin >= 40) return false;
  ButtonState& b = _buttons[pin];
  const uint32_t now = millis();
  if (!b.ready) {
    releasePWM(pin);
    ::pinMode(pin, INPUT_PULLUP);
    b.ready = true;
    b.stable = b.last = true; // Released (HIGH)
    b.changedAt = now;
    return false;
  }
  const bool level = ::digitalRead(pin) == HIGH;
  if (level != b.last) {
    b.last = level;
    b.changedAt = now;
  }
  if (level != b.stable && uint32_t(now - b.changedAt) >= BUTTON_DEBOUNCE_MS) {
    b.stable = level;
    return !level; // Pressed = pulled to GND
  }
  return false;
}

// =============================================================================
// MCP3208 12-bit SPI ADC
// =============================================================================
uint16_t RBNexusBoard::analogRaw(uint8_t channel) {
  if (channel < RB_ADC_CH_MIN || channel > RB_ADC_CH_MAX) return 0;
  const uint8_t mcpCh = channel - 1; // A1 -> CH0, ..., A8 -> CH7

  // MCP3208 frame: start(1) SGL(1) D2 | D1 D0 in bits 7-6 | clock out
  uint8_t tx0 = 0x06 | ((mcpCh >> 2) & 0x01);
  uint8_t tx1 = (mcpCh & 0x03) << 6;

  SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
  ::digitalWrite(RB_PIN_SPI_CS, LOW);
  SPI.transfer(tx0);
  uint8_t b1 = SPI.transfer(tx1);
  uint8_t b2 = SPI.transfer(0x00);
  ::digitalWrite(RB_PIN_SPI_CS, HIGH);
  SPI.endTransaction();

  return ((uint16_t)(b1 & 0x0F) << 8) | b2;
}

float RBNexusBoard::analogVolt(uint8_t channel, float vref) {
  return (analogRaw(channel) * vref) / 4095.0f;
}

float RBNexusBoard::analogPercent(uint8_t channel) {
  return analogRaw(channel) * 100.0f / 4095.0f;
}

uint16_t RBNexusBoard::analogAverage(uint8_t channel, uint16_t samples) {
  if (samples == 0) return 0;
  uint32_t sum = 0;
  for (uint16_t i = 0; i < samples; ++i) {
    sum += analogRaw(channel);
    delayMicroseconds(50);
  }
  return (uint16_t)(sum / samples);
}

// =============================================================================
// PCA9685
// =============================================================================
uint8_t RBNexusBoard::readRegister8(uint8_t reg) {
  Wire.beginTransmission(RB_PCA9685_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(RB_PCA9685_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

void RBNexusBoard::writeRegister8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(RB_PCA9685_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void RBNexusBoard::pcaFrequency(float freqHz) {
  if (!_pcaConnected || !isfinite(freqHz)) return;
  if (freqHz < 24.0f) freqHz = 24.0f;
  if (freqHz > 1526.0f) freqHz = 1526.0f;

  float prescaleval = (25000000.0f / (4096.0f * freqHz)) - 1.0f;
  uint8_t prescale = (uint8_t)(prescaleval + 0.5f);
  _pwmFrequency = 25000000.0f / (4096.0f * (prescale + 1));

  uint8_t oldmode = readRegister8(PCA9685_MODE1);
  uint8_t sleepmode = (oldmode & 0x7F) | 0x10;
  writeRegister8(PCA9685_MODE1, sleepmode);
  writeRegister8(PCA9685_PRESCALE, prescale);
  writeRegister8(PCA9685_MODE1, oldmode);
  delayMicroseconds(500);
  writeRegister8(PCA9685_MODE1, oldmode | 0xA0);
}

void RBNexusBoard::pcaWrite(uint8_t channel, uint16_t on, uint16_t off) {
  if (!_pcaConnected || channel > 15) return;
  Wire.beginTransmission(RB_PCA9685_ADDR);
  Wire.write(PCA9685_LED0_ON_L + 4 * channel);
  Wire.write(on & 0xFF);
  Wire.write((on >> 8) & 0xFF);
  Wire.write(off & 0xFF);
  Wire.write((off >> 8) & 0xFF);
  Wire.endTransmission();
}

void RBNexusBoard::pcaDuty(uint8_t channel, uint16_t duty) {
  if (channel > 15) return;
  if (duty == 0) {
    pcaWrite(channel, 0, 4096);      // Full OFF
  } else if (duty >= 4095) {
    pcaWrite(channel, 4096, 0);      // Full ON
  } else {
    pcaWrite(channel, 0, duty);
  }
}

// =============================================================================
// DC motors with dead-time protection
// =============================================================================
void RBNexusBoard::motorPower(uint8_t motorId, int16_t percent) {
  if (!validMotor(motorId)) return;
  int32_t p = clampPercent(percent);
  if (_motorReversed[motorId - 1]) p = -p;
  motorSpeed255(motorId, int16_t((p * 255) / 100));
}

void RBNexusBoard::motorSpeed255(uint8_t motorId, int16_t speed) {
  if (!validMotor(motorId)) return;
  _pid[motorId - 1].enabled = false;
  motorOutputSet(motorId, speed);
}

void RBNexusBoard::motorOutputSet(uint8_t motorId, int16_t speed) {
  if (!validMotor(motorId)) return;
  if (_emergencyStopActive) {
    motorOutputStop(motorId);
    return;
  }

  uint8_t idx = motorId - 1;
  uint8_t chA = idx * 2;
  uint8_t chB = chA + 1;

  if (speed == 0) {
    motorOutputStop(motorId);
    return;
  }

  int8_t newDir = (speed > 0) ? 1 : -1;

  // Direction reversal dead-time protection
  if (_motorLastDir[idx] != 0 && _motorLastDir[idx] != newDir) {
    pcaDuty(chA, 0);
    pcaDuty(chB, 0);
    delay(_deadTimeMs);
  } else if (_motorLastDir[idx] == 0) {
    uint32_t elapsed = millis() - _motorLastStopTime[idx];
    if (elapsed < _deadTimeMs) delay(_deadTimeMs - elapsed);
  }

  int32_t absSpeed = speed < 0 ? -(int32_t)speed : speed;
  if (absSpeed > 255) absSpeed = 255;
  uint16_t duty = (uint16_t)((absSpeed * 4095UL) / 255);

  if (newDir > 0) {
    pcaDuty(chA, duty);
    pcaDuty(chB, 0);
  } else {
    pcaDuty(chA, 0);
    pcaDuty(chB, duty);
  }

  _motorLastDir[idx] = newDir;
}

void RBNexusBoard::motorStop(uint8_t motorId) {
  if (!validMotor(motorId)) return;
  auto& pid = _pid[motorId - 1];
  pid.enabled = false;
  pid.targetRPM = pid.integral = pid.prevError = 0;
  motorOutputStop(motorId);
}

void RBNexusBoard::motorOutputStop(uint8_t motorId) {
  if (!validMotor(motorId)) return;
  uint8_t idx = motorId - 1;
  uint8_t chA = idx * 2;
  uint8_t chB = chA + 1;
  pcaDuty(chA, 0);
  pcaDuty(chB, 0);
  _motorLastDir[idx] = 0;
  _motorLastStopTime[idx] = millis();
}

void RBNexusBoard::motorBrake(uint8_t motorId) {
  if (!validMotor(motorId)) return;
  motorStop(motorId);
  if (_emergencyStopActive) return;
  uint8_t idx = motorId - 1;
  uint8_t chA = idx * 2;
  uint8_t chB = chA + 1;
  pcaDuty(chA, 4095);
  pcaDuty(chB, 4095);
  _motorLastDir[idx] = 0;
}

void RBNexusBoard::emergencyStop() {
  _emergencyStopActive = true;
  stop();
  ::digitalWrite(RB_PIN_LED, LOW);
}

// =============================================================================
// Closed-loop PID motor control
// =============================================================================
void RBNexusBoard::motorRPM(uint8_t motorId, float rpm) {
  if (!validMotor(motorId)) return;
  uint8_t idx = motorId - 1;
  if (_emergencyStopActive || !isfinite(rpm)) return;
  if (rpm == 0) { motorStop(motorId); return; }
  if (!_pid[idx].enabled) {
    _pid[idx].integral = _pid[idx].prevError = 0;
    _pid[idx].primed = false; // Avoid a derivative kick on the first sample
    _pid[idx].lastTime = millis();
  }
  _pid[idx].targetRPM = rpm;
  _pid[idx].enabled = true;
}

void RBNexusBoard::motorPID(uint8_t motorId, float kp, float ki, float kd) {
  if (!validMotor(motorId)) return;
  if (!isfinite(kp) || !isfinite(ki) || !isfinite(kd)) return;
  uint8_t idx = motorId - 1;
  _pid[idx].kp = kp;
  _pid[idx].ki = ki;
  _pid[idx].kd = kd;
}

bool RBNexusBoard::motorPIDActive(uint8_t motorId) const {
  return validMotor(motorId) && _pid[motorId - 1].enabled;
}

void RBNexusBoard::updatePID() {
  unsigned long now = millis();
  for (int i = 0; i < 4; ++i) {
    if (!_pid[i].enabled || _emergencyStopActive) continue;

    float dt = (now - _pid[i].lastTime) / 1000.0f;
    if (dt <= 0.0f || dt > 0.5f) dt = 0.05f;
    _pid[i].lastTime = now;

    float error = _pid[i].targetRPM - _currentRPM[i];
    if (!_pid[i].primed) {
      _pid[i].prevError = error;
      _pid[i].primed = true;
    }
    _pid[i].integral += error * dt;

    // Integral anti-windup clamp
    if (_pid[i].integral > 255.0f) _pid[i].integral = 255.0f;
    if (_pid[i].integral < -255.0f) _pid[i].integral = -255.0f;

    float derivative = (error - _pid[i].prevError) / dt;
    _pid[i].prevError = error;

    float output = (_pid[i].kp * error) + (_pid[i].ki * _pid[i].integral) + (_pid[i].kd * derivative);

    if (output > 255.0f) output = 255.0f;
    if (output < -255.0f) output = -255.0f;

    motorOutputSet(i + 1, (int16_t)output);
  }
}

// =============================================================================
// Servo 1-8
// =============================================================================
void RBNexusBoard::servoChannelPulse(uint8_t channel, uint16_t us) {
  if (channel < RB_SERVO_CH_MIN || channel > RB_SERVO_CH_MAX) return;
  if (us == 0) { pcaDuty(channel, 0); return; }
  if (us < 500) us = 500;
  if (us > 2500) us = 2500;
  uint32_t ticks = (uint32_t)(us * _pwmFrequency * 4096.0f / 1000000.0f + 0.5f);
  if (ticks > 4095) ticks = 4095;
  pcaWrite(channel, 0, ticks);
}

void RBNexusBoard::servoPulse(uint8_t servo, uint16_t microseconds) {
  if (!validServo(servo)) return;
  servoChannelPulse(RB_SERVO_CH_MIN + servo - 1, microseconds);
}

void RBNexusBoard::servoAngle(uint8_t servo, uint8_t degrees, uint16_t minUs, uint16_t maxUs) {
  if (!validServo(servo) || minUs > maxUs) return;
  if (degrees > 180) degrees = 180;
  uint16_t us = minUs + (uint16_t)(((uint32_t)degrees * (maxUs - minUs)) / 180UL);
  servoChannelPulse(RB_SERVO_CH_MIN + servo - 1, us);
}

void RBNexusBoard::servoRelease(uint8_t servo) {
  if (!validServo(servo)) return;
  pcaDuty(RB_SERVO_CH_MIN + servo - 1, 0);
}

// =============================================================================
// Quadrature encoders
// =============================================================================
void RBNexusBoard::encoderBegin() {
  if (_encodersInitialized) return;

  ::pinMode(RB_PIN_ENC1_A, INPUT);
  ::pinMode(RB_PIN_ENC1_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(RB_PIN_ENC1_A), isrEnc1A, CHANGE);

  ::pinMode(RB_PIN_ENC2_A, INPUT);
  ::pinMode(RB_PIN_ENC2_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(RB_PIN_ENC2_A), isrEnc2A, CHANGE);

  ::pinMode(RB_PIN_ENC3_A, INPUT_PULLUP);
  ::pinMode(RB_PIN_ENC3_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RB_PIN_ENC3_A), isrEnc3A, CHANGE);

  ::pinMode(RB_PIN_ENC4_A, INPUT_PULLUP);
  ::pinMode(RB_PIN_ENC4_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RB_PIN_ENC4_A), isrEnc4A, CHANGE);

  unsigned long now = millis();
  for (int i = 0; i < 4; ++i) {
    _lastEncTicks[i] = s_encTicks[i];
    _lastEncTime[i] = now;
  }

  _encodersInitialized = true;
}

int32_t RBNexusBoard::encoderTicks(uint8_t encId) {
  if (encId < 1 || encId > 4) return 0;
  return s_encTicks[encId - 1];
}

void RBNexusBoard::encoderReset(uint8_t encId) {
  if (encId < 1 || encId > 4) return;
  s_encTicks[encId - 1] = 0;
  _lastEncTicks[encId - 1] = 0;
}

void RBNexusBoard::encoderResetAll() {
  for (uint8_t id = 1; id <= 4; ++id) encoderReset(id);
}

float RBNexusBoard::encoderRPM(uint8_t encId) {
  if (encId < 1 || encId > 4) return 0.0f;
  return _currentRPM[encId - 1];
}

void RBNexusBoard::encoderSetPPR(uint8_t encId, uint16_t ticksPerRev) {
  if (encId < 1 || encId > 4 || ticksPerRev == 0) return;
  _encPPR[encId - 1] = ticksPerRev;
}

void RBNexusBoard::updateEncodersRPM() {
  unsigned long now = millis();
  for (int i = 0; i < 4; ++i) {
    int32_t currentTicks = s_encTicks[i];
    int32_t deltaTicks = currentTicks - _lastEncTicks[i];
    float dt = (now - _lastEncTime[i]) / 1000.0f;

    if (dt >= 0.04f && _encPPR[i] > 0) {
      float revs = (float)deltaTicks / (float)_encPPR[i];
      float rawRPM = (revs / dt) * 60.0f;

      // Low-pass filter for a smooth RPM readout
      _currentRPM[i] = (_currentRPM[i] * 0.7f) + (rawRPM * 0.3f);
      _lastEncTicks[i] = currentTicks;
      _lastEncTime[i] = now;
    }
  }
}

// =============================================================================
// I2C
// =============================================================================
int RBNexusBoard::i2cScan(Print* out) {
  int found = 0;
  if (out) out->println("Scanning I2C bus (SDA=21, SCL=22)...");

  for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (out) {
        out->printf("  - Found device at 0x%02X", addr);
        if (addr == RB_PCA9685_ADDR) out->print(" (PCA9685 PWM Controller)");
        else if (addr == 0x68 || addr == 0x69) out->print(" (Possible MPU family; read WHO_AM_I at register 0x75)");
        else if (addr == 0x4A || addr == 0x4B) out->print(" (Possible BNO085; identity not verified)");
        out->println();
      }
      found++;
    }
  }
  if (out) out->printf("Total I2C devices: %d\n", found);
  return found;
}

// =============================================================================
// CAN Bus (TWAI)
// =============================================================================
bool RBNexusBoard::canBegin(uint32_t baudRate, int txPin, int rxPin) {
  if (txPin < 0 || rxPin < 0) {
    Serial.println("[CAN] TX/RX pins must be configured with valid GPIO numbers.");
    _canInitialized = false;
    return false;
  }

  if (_canInitialized) canStop();
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)txPin, (gpio_num_t)rxPin, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config;

  switch (baudRate) {
    case 125000:  t_config = TWAI_TIMING_CONFIG_125KBITS(); break;
    case 250000:  t_config = TWAI_TIMING_CONFIG_250KBITS(); break;
    case 500000:  t_config = TWAI_TIMING_CONFIG_500KBITS(); break;
    case 1000000: t_config = TWAI_TIMING_CONFIG_1MBITS(); break;
    default: return false;
  }

  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    _canInitialized = false;
    return false;
  }
  if (twai_start() != ESP_OK) {
    twai_driver_uninstall();
    _canInitialized = false;
    return false;
  }
  _canInitialized = true;
  return true;
}

bool RBNexusBoard::canSend(uint32_t id, const uint8_t* data, uint8_t len, bool ext) {
  if (!_canInitialized || len > 8 || (len && !data) ||
      id > (ext ? 0x1FFFFFFFUL : 0x7FFUL)) return false;
  twai_message_t msg = {};
  msg.identifier = id;
  msg.extd = ext ? 1 : 0;
  msg.rtr = 0;
  msg.data_length_code = len;
  for (uint8_t i = 0; i < len; ++i) {
    msg.data[i] = data[i];
  }
  return twai_transmit(&msg, pdMS_TO_TICKS(50)) == ESP_OK;
}

bool RBNexusBoard::canReceive(uint32_t& id, uint8_t* data, uint8_t& len, bool& ext) {
  if (!_canInitialized || !data) return false;
  twai_message_t msg = {};
  if (twai_receive(&msg, pdMS_TO_TICKS(10)) != ESP_OK) return false;
  // Ignore remote frames without touching the caller's variables.
  if (msg.rtr || msg.data_length_code > 8) return false;
  id = msg.identifier;
  ext = (msg.extd == 1);
  len = msg.data_length_code;
  for (uint8_t i = 0; i < len; ++i) {
    data[i] = msg.data[i];
  }
  return true;
}

void RBNexusBoard::canStop() {
  if (_canInitialized) {
    twai_stop();
    twai_driver_uninstall();
    _canInitialized = false;
  }
}

// =============================================================================
// Wi-Fi
// =============================================================================
bool RBNexusBoard::wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

int8_t RBNexusBoard::wifiRSSI() {
  return WiFi.RSSI();
}

String RBNexusBoard::wifiIP() {
  return WiFi.localIP().toString();
}

bool RBNexusBoard::wifiConnect(const char* ssid, const char* pass, uint32_t timeoutMs) {
  if (!ssid) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < timeoutMs)) {
    delay(100);
  }
  return WiFi.status() == WL_CONNECTED;
}

// =============================================================================
// Safety watchdog
// =============================================================================
void RBNexusBoard::watchdogStart(uint32_t timeoutMs) {
  _watchdogEnabled = true;
  _watchdogTimeoutMs = timeoutMs;
  watchdogFeed();
}

void RBNexusBoard::watchdogFeed() {
  _lastWatchdogFeed = millis();
}

// Global instance
RBNexusBoard RB;

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
static constexpr uint8_t PCA9685_MODE1       = 0x00;
static constexpr uint8_t PCA9685_MODE2       = 0x01;
static constexpr uint8_t PCA9685_LED0_ON_L   = 0x06;
static constexpr uint8_t PCA9685_ALLLED_ON_L = 0xFA;
static constexpr uint8_t PCA9685_PRESCALE    = 0xFE;

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

bool RBNexusBoard::begin(uint32_t i2cFreq, uint32_t spiFreq) {
  _spiFreq = spiFreq;

  // Initialize Status LED
  pinMode(RB_PIN_LED, OUTPUT);
  digitalWrite(RB_PIN_LED, LOW);

  // Initialize I2C Bus for PCA9685
  Wire.begin(RB_PIN_I2C_SDA, RB_PIN_I2C_SCL, i2cFreq);

  // Probe PCA9685
  Wire.beginTransmission(RB_PCA9685_ADDR);
  _pcaConnected = (Wire.endTransmission() == 0);

  if (_pcaConnected) {
    writeRegister8(PCA9685_MODE1, 0x20); // Auto-increment enabled
    writeRegister8(PCA9685_MODE2, 0x04); // Totem-pole output
    setPWMFreq(50.0f);                  // 50 Hz default for Servos and Motors
    for (uint8_t ch = 0; ch < 16; ++ch) {
      setPWMDuty(ch, 0);
    }
  }

  // Initialize SPI Bus for MCP3208
  pinMode(RB_PIN_SPI_CS, OUTPUT);
  digitalWrite(RB_PIN_SPI_CS, HIGH);
  SPI.begin(RB_PIN_SPI_CLK, RB_PIN_SPI_MISO, RB_PIN_SPI_MOSI, RB_PIN_SPI_CS);

  feedWatchdog();
  return _pcaConnected;
}

void RBNexusBoard::update() {
  unsigned long now = millis();

  // Watchdog check
  if (_watchdogEnabled && (now - _lastWatchdogFeed > _watchdogTimeoutMs)) {
    if (!_emergencyStopActive) {
      emergencyStop();
    }
  }

  // Encoders RPM computation (every 50 ms)
  if (now - _lastRPMUpdate >= 50) {
    _lastRPMUpdate = now;
    updateEncodersRPM();
    updatePID();
  }

  imuUpdate();

  // Background Wi-Fi Auto-reconnect check (every 5 seconds)
  if (_wifiAutoReconnect && (now - _lastWiFiCheck >= 5000)) {
    _lastWiFiCheck = now;
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_OFF) {
      WiFi.reconnect();
    }
  }
}

// --- Status LED ---
void RBNexusBoard::setLED(bool on) {
  digitalWrite(RB_PIN_LED, on ? HIGH : LOW);
}

void RBNexusBoard::toggleLED() {
  digitalWrite(RB_PIN_LED, !digitalRead(RB_PIN_LED));
}

// --- Digital I/O Helpers ---
int RBNexusBoard::digitalRead(uint8_t pin) {
  return ::digitalRead(pin);
}

void RBNexusBoard::digitalWrite(uint8_t pin, uint8_t val) {
  ::digitalWrite(pin, val);
}

void RBNexusBoard::pinMode(uint8_t pin, uint8_t mode) {
  ::pinMode(pin, mode);
}

// --- ESP32 Internal LEDC PWM ---
bool RBNexusBoard::pwmWrite(uint8_t pin, uint32_t duty, uint32_t freq, uint8_t resolution) {
  if (pin >= 40 || resolution < 1 || resolution > 20 || freq == 0) return false;
  if (_pwmResolution[pin] == 0) {
    if (!ledcAttach(pin, freq, resolution)) return false;
  } else if (_pwmResolution[pin] != resolution || ledcReadFreq(pin) != freq) {
    if (ledcChangeFrequency(pin, freq, resolution) == 0) return false;
  }
  _pwmResolution[pin] = resolution;
  const uint32_t maximum = (1UL << resolution) - 1;
  return ledcWrite(pin, duty > maximum ? maximum : duty);
}

bool RBNexusBoard::pwmSetFrequency(uint8_t pin, uint32_t hz) {
  return pin < 40 && _pwmResolution[pin] && hz &&
         ledcChangeFrequency(pin, hz, _pwmResolution[pin]) != 0;
}

void RBNexusBoard::pwmStop(uint8_t pin) {
  if (pin >= 40) return;
  ledcDetach(pin);
  _pwmResolution[pin] = 0;
  ::pinMode(pin, OUTPUT);
  ::digitalWrite(pin, LOW);
}

// --- MCP3208 12-bit SPI ADC ---
uint16_t RBNexusBoard::analogRead(uint8_t channel) {
  uint8_t mcpCh = 0;
  if (channel >= RB_ADC_CH_MIN && channel <= RB_ADC_CH_MAX) {
    mcpCh = channel - 1; // A1 -> CH0, ..., A8 -> CH7
  } else {
    return 0;
  }

  // MCP3208 SPI command frame:
  // Byte 0: Start bit (1) | SGL (1) | D2
  // Byte 1: D1 | D0 in bits 7-6
  // Byte 2: 0x00 clock out
  uint8_t tx[3];
  tx[0] = 0x06 | ((mcpCh >> 2) & 0x01);
  tx[1] = (mcpCh & 0x03) << 6;
  tx[2] = 0x00;

  SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
  ::digitalWrite(RB_PIN_SPI_CS, LOW);
  SPI.transfer(tx[0]);
  uint8_t b1 = SPI.transfer(tx[1]);
  uint8_t b2 = SPI.transfer(tx[2]);
  ::digitalWrite(RB_PIN_SPI_CS, HIGH);
  SPI.endTransaction();

  return ((uint16_t)(b1 & 0x0F) << 8) | b2;
}

float RBNexusBoard::analogVoltage(uint8_t channel, float vref) {
  uint16_t raw = analogRead(channel);
  return (raw * vref) / 4095.0f;
}

uint16_t RBNexusBoard::analogReadAverage(uint8_t channel, uint16_t samples) {
  if (samples == 0) return 0;
  uint32_t sum = 0;
  for (uint16_t i = 0; i < samples; ++i) {
    sum += analogRead(channel);
    delayMicroseconds(50);
  }
  return (uint16_t)(sum / samples);
}

uint16_t RBNexusBoard::analogReadMin(uint8_t channel, uint16_t samples) {
  if (samples == 0) return 0;
  uint16_t minVal = 4095;
  for (uint16_t i = 0; i < samples; ++i) {
    uint16_t val = analogRead(channel);
    if (val < minVal) minVal = val;
    delayMicroseconds(50);
  }
  return minVal;
}

uint16_t RBNexusBoard::analogReadMax(uint8_t channel, uint16_t samples) {
  if (samples == 0) return 0;
  uint16_t maxVal = 0;
  for (uint16_t i = 0; i < samples; ++i) {
    uint16_t val = analogRead(channel);
    if (val > maxVal) maxVal = val;
    delayMicroseconds(50);
  }
  return maxVal;
}

// --- PCA9685 Low-level PWM ---
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

void RBNexusBoard::setPWMFreq(float freqHz) {
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

void RBNexusBoard::setPWM(uint8_t channel, uint16_t on, uint16_t off) {
  if (!_pcaConnected || channel > 15) return;
  Wire.beginTransmission(RB_PCA9685_ADDR);
  Wire.write(PCA9685_LED0_ON_L + 4 * channel);
  Wire.write(on & 0xFF);
  Wire.write((on >> 8) & 0xFF);
  Wire.write(off & 0xFF);
  Wire.write((off >> 8) & 0xFF);
  Wire.endTransmission();
}

void RBNexusBoard::setPWMDuty(uint8_t channel, uint16_t duty) {
  if (channel > 15) return;
  if (duty == 0) {
    setPWM(channel, 0, 4096);
  } else if (duty >= 4095) {
    setPWM(channel, 4096, 0);
  } else {
    setPWM(channel, 0, duty);
  }
}

// --- DC Motor Control with Dead-time Safety ---
void RBNexusBoard::motorSet(uint8_t motorId, int16_t speed) {
  if (motorId < 1 || motorId > 4) return;
  _pid[motorId - 1].enabled = false;
  motorOutputSet(motorId, speed);
}

void RBNexusBoard::motorOutputSet(uint8_t motorId, int16_t speed) {
  if (motorId < 1 || motorId > 4) return;
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

  // Direction Reversal Dead-time Protection
  if (_motorLastDir[idx] != 0 && _motorLastDir[idx] != newDir) {
    setPWMDuty(chA, 0);
    setPWMDuty(chB, 0);
    delay(_deadTimeMs);
  } else if (_motorLastDir[idx] == 0) {
    uint32_t elapsed = millis() - _motorLastStopTime[idx];
    if (elapsed < _deadTimeMs) delay(_deadTimeMs - elapsed);
  }

  int32_t absSpeed = speed < 0 ? -(int32_t)speed : speed;
  if (absSpeed > 255) absSpeed = 255;
  uint16_t duty = (uint16_t)((absSpeed * 4095UL) / 255);

  if (newDir > 0) {
    setPWMDuty(chA, duty);
    setPWMDuty(chB, 0);
  } else {
    setPWMDuty(chA, 0);
    setPWMDuty(chB, duty);
  }

  _motorLastDir[idx] = newDir;
}

void RBNexusBoard::motorForward(uint8_t motorId, uint8_t speed) {
  motorSet(motorId, (int16_t)speed);
}

void RBNexusBoard::motorBackward(uint8_t motorId, uint8_t speed) {
  motorSet(motorId, -(int16_t)speed);
}

void RBNexusBoard::motorStop(uint8_t motorId) {
  if (motorId < 1 || motorId > 4) return;
  auto& pid = _pid[motorId - 1];
  pid.enabled = false;
  pid.targetRPM = pid.integral = pid.prevError = 0;
  motorOutputStop(motorId);
}

void RBNexusBoard::motorOutputStop(uint8_t motorId) {
  if (motorId < 1 || motorId > 4) return;
  uint8_t idx = motorId - 1;
  uint8_t chA = idx * 2;
  uint8_t chB = chA + 1;
  setPWMDuty(chA, 0);
  setPWMDuty(chB, 0);
  _motorLastDir[idx] = 0;
  _motorLastStopTime[idx] = millis();
}

void RBNexusBoard::motorBrake(uint8_t motorId) {
  if (motorId < 1 || motorId > 4) return;
  motorStop(motorId);
  if (_emergencyStopActive) return;
  uint8_t idx = motorId - 1;
  uint8_t chA = idx * 2;
  uint8_t chB = chA + 1;
  setPWMDuty(chA, 4095);
  setPWMDuty(chB, 4095);
  _motorLastDir[idx] = 0;
}

void RBNexusBoard::motorStopAll() {
  for (uint8_t i = 1; i <= 4; ++i) {
    motorStop(i);
  }
}

void RBNexusBoard::emergencyStop() {
  _emergencyStopActive = true;
  motorStopAll();
  digitalWrite(RB_PIN_LED, LOW);
}

// --- Closed-Loop PID Motor Control ---
void RBNexusBoard::motorSetRPM(uint8_t motorId, float rpm) {
  if (motorId < 1 || motorId > 4) return;
  uint8_t idx = motorId - 1;
  if (_emergencyStopActive || !isfinite(rpm)) return;
  if (rpm == 0) { motorStop(motorId); return; }
  if (!_pid[idx].enabled) {
    _pid[idx].integral = _pid[idx].prevError = 0;
    _pid[idx].lastTime = millis();
  }
  _pid[idx].targetRPM = rpm;
  _pid[idx].enabled = true;
}

void RBNexusBoard::motorSetPID(uint8_t motorId, float kp, float ki, float kd) {
  if (motorId < 1 || motorId > 4) return;
  uint8_t idx = motorId - 1;
  if (!isfinite(kp) || !isfinite(ki) || !isfinite(kd)) return;
  _pid[idx].kp = kp;
  _pid[idx].ki = ki;
  _pid[idx].kd = kd;
}

void RBNexusBoard::motorPIDEnable(uint8_t motorId) {
  if (motorId < 1 || motorId > 4) return;
  if (_emergencyStopActive) return;
  motorSetRPM(motorId, _pid[motorId - 1].targetRPM);
}

void RBNexusBoard::motorPIDDisable(uint8_t motorId) {
  if (motorId < 1 || motorId > 4) return;
  _pid[motorId - 1].enabled = false;
  motorStop(motorId);
}

bool RBNexusBoard::isMotorPIDEnabled(uint8_t motorId) const {
  if (motorId < 1 || motorId > 4) return false;
  return _pid[motorId - 1].enabled;
}

void RBNexusBoard::updatePID() {
  unsigned long now = millis();
  for (int i = 0; i < 4; ++i) {
    if (!_pid[i].enabled || _emergencyStopActive) continue;

    float dt = (now - _pid[i].lastTime) / 1000.0f;
    if (dt <= 0.0f || dt > 0.5f) dt = 0.05f;
    _pid[i].lastTime = now;

    float error = _pid[i].targetRPM - _currentRPM[i];
    _pid[i].integral += error * dt;

    // Integral anti-windup clamp
    if (_pid[i].integral > 255.0f) _pid[i].integral = 255.0f;
    if (_pid[i].integral < -255.0f) _pid[i].integral = -255.0f;

    float derivative = (error - _pid[i].prevError) / dt;
    _pid[i].prevError = error;

    float output = (_pid[i].kp * error) + (_pid[i].ki * _pid[i].integral) + (_pid[i].kd * derivative);

    // Clamp output to robotics speed scale (-255 to 255)
    if (output > 255.0f) output = 255.0f;
    if (output < -255.0f) output = -255.0f;

    motorOutputSet(i + 1, (int16_t)output);
  }
}

// --- Servo Control ---
void RBNexusBoard::servoAttach(uint8_t channel) {
  if (channel < RB_SERVO_CH_MIN || channel > RB_SERVO_CH_MAX) return;
  servoWrite(channel, 90);
}

void RBNexusBoard::servoWriteMicroseconds(uint8_t channel, uint16_t us) {
  if (channel < RB_SERVO_CH_MIN || channel > RB_SERVO_CH_MAX) return;
  if (us == 0) { servoDetach(channel); return; }
  if (us < 500) us = 500;
  if (us > 2500) us = 2500;
  uint32_t ticks = (uint32_t)(us * _pwmFrequency * 4096.0f / 1000000.0f + 0.5f);
  if (ticks > 4095) ticks = 4095;
  setPWM(channel, 0, ticks);
}

void RBNexusBoard::servoWrite(uint8_t channel, uint8_t angle, uint16_t minUs, uint16_t maxUs) {
  if (minUs > maxUs) return;
  if (angle > 180) angle = 180;
  uint16_t us = minUs + (uint16_t)(((uint32_t)angle * (maxUs - minUs)) / 180UL);
  servoWriteMicroseconds(channel, us);
}

void RBNexusBoard::servoDetach(uint8_t channel) {
  if (channel < RB_SERVO_CH_MIN || channel > RB_SERVO_CH_MAX) return;
  setPWMDuty(channel, 0);
}

// --- Quadrature Encoders ---
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

int32_t RBNexusBoard::encoderRead(uint8_t encId) {
  if (encId < 1 || encId > 4) return 0;
  return s_encTicks[encId - 1];
}

void RBNexusBoard::encoderReset(uint8_t encId) {
  if (encId < 1 || encId > 4) return;
  s_encTicks[encId - 1] = 0;
  _lastEncTicks[encId - 1] = 0;
}

void RBNexusBoard::encoderResetAll() {
  for (uint8_t i = 0; i < 4; ++i) {
    s_encTicks[i] = 0;
    _lastEncTicks[i] = 0;
  }
}

float RBNexusBoard::encoderRPM(uint8_t encId) {
  if (encId < 1 || encId > 4) return 0.0f;
  return _currentRPM[encId - 1];
}

int8_t RBNexusBoard::encoderDirection(uint8_t encId) {
  if (encId < 1 || encId > 4) return 0;
  float rpm = _currentRPM[encId - 1];
  if (rpm > 0.5f) return 1;
  if (rpm < -0.5f) return -1;
  return 0;
}

void RBNexusBoard::encoderSetPPR(uint8_t encId, uint16_t ppr) {
  if (encId < 1 || encId > 4 || ppr == 0) return;
  _encPPR[encId - 1] = ppr;
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

      // Low-pass filter for smooth RPM readout
      _currentRPM[i] = (_currentRPM[i] * 0.7f) + (rawRPM * 0.3f);
      _lastEncTicks[i] = currentTicks;
      _lastEncTime[i] = now;
    }
  }
}

// --- I2C Bus Utilities ---
bool RBNexusBoard::i2cBegin(uint32_t freq) {
  return Wire.begin(RB_PIN_I2C_SDA, RB_PIN_I2C_SCL, freq);
}

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

// --- CAN Bus (TWAI) Implementation ---
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
    case 1000000: t_config = TWAI_TIMING_CONFIG_1MBITS(); break;
    case 500000: t_config = TWAI_TIMING_CONFIG_500KBITS(); break;
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
  if (twai_receive(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
    id = msg.identifier;
    ext = (msg.extd == 1);
    if (msg.rtr || msg.data_length_code > 8) return false;
    len = msg.data_length_code;
    for (uint8_t i = 0; i < len; ++i) {
      data[i] = msg.data[i];
    }
    return true;
  }
  return false;
}

void RBNexusBoard::canStop() {
  if (_canInitialized) {
    twai_stop();
    twai_driver_uninstall();
    _canInitialized = false;
  }
}

// --- Wi-Fi Utilities ---
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < timeoutMs)) {
    delay(100);
  }
  return WiFi.status() == WL_CONNECTED;
}

// --- Safety Watchdog ---
void RBNexusBoard::enableWatchdog(uint32_t timeoutMs) {
  _watchdogEnabled = true;
  _watchdogTimeoutMs = timeoutMs;
  feedWatchdog();
}

void RBNexusBoard::feedWatchdog() {
  _lastWatchdogFeed = millis();
}

// Global instances
RBNexusBoard RB;
RBNexusBoard& RB_Nexus = RB;

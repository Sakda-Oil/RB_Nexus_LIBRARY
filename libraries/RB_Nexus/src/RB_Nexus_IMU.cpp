// SPDX-License-Identifier: LGPL-2.1-or-later
#include "RB_Nexus.h"
#include <MPU9250.h>
#include <Adafruit_BNO08x.h>
#include <math.h>

namespace {
MPU9250 mpu;
Adafruit_BNO08x bno(-1); // I2C soft reset; no extra GPIO required.
constexpr uint8_t ACCEL_VALID = 1, GYRO_VALID = 2, ORIENTATION_VALID = 4;

bool probe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool readIMURegisters(uint8_t address, uint8_t reg, uint8_t* data, uint8_t count) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(address, count) != count) return false;
  for (uint8_t i = 0; i < count; ++i) data[i] = Wire.read();
  return true;
}

bool hasIdentity(uint8_t address, uint8_t expected) {
  uint8_t identity = 0;
  return readIMURegisters(address, 0x75, &identity, 1) && identity == expected;
}

bool writeIMURegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg); Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool setupMPU6050(uint8_t address) {
  if (!hasIdentity(address, 0x68)) return false;
  if (!writeIMURegister(address, 0x6B, 0x80)) return false;
  delay(100);
  // PLL X gyro, all axes enabled, DLPF ~42/44 Hz, 200 Hz sampling.
  return writeIMURegister(address, 0x6B, 0x01) &&
         writeIMURegister(address, 0x6C, 0x00) &&
         writeIMURegister(address, 0x1A, 0x03) &&
         writeIMURegister(address, 0x19, 0x04) &&
         writeIMURegister(address, 0x1B, 0x08) && // +/-500 degrees/s
         writeIMURegister(address, 0x1C, 0x08) && // +/-4g
         writeIMURegister(address, 0x38, 0x01);   // DATA_RDY status, polled
}

bool enableBNOReports() {
  return bno.enableReport(SH2_ACCELEROMETER, 20000) &&
         bno.enableReport(SH2_GYROSCOPE_CALIBRATED, 20000) &&
         bno.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, 50000) &&
         bno.enableReport(SH2_ROTATION_VECTOR, 20000);
}
}

bool RBNexusBoard::imuBegin(RBIMUType type, uint8_t address) {
  // The vendor SH2 implementation owns one global transport.
  if (this != &RB) return false;
  _imuDetected = false;
  _imuModel = "None";
  _imuType = RBIMUType::Auto;
  _imuReports = 0;
  _imuAddress = 0;
  for (int i = 0; i < 3; ++i) _accel[i] = _gyro[i] = _mag[i] = _angles[i] = NAN;
  for (float& q : _quaternion) q = NAN;
  Wire.setTimeOut(20);

  // Prefer BNO085 if both modules are fitted. Explicit selection overrides this.
  if (type == RBIMUType::Auto || type == RBIMUType::BNO085) {
    for (uint8_t addr : {uint8_t(0x4A), uint8_t(0x4B)}) {
      if (address && address != addr) continue;
      if (probe(addr) && bno.begin_I2C(addr, &Wire) && enableBNOReports()) {
        _imuDetected = true;
        _imuType = RBIMUType::BNO085;
        _imuModel = "BNO085 / SH2";
        _imuAddress = addr;
        bno.wasReset(); // Initial reset already handled by enableBNOReports().
        return true;
      }
    }
  }
  if (type == RBIMUType::Auto || type == RBIMUType::MPU9250) {
    for (uint8_t addr : {uint8_t(0x68), uint8_t(0x69)}) {
      if (address && address != addr) continue;
      if (!hasIdentity(addr, 0x71)) continue;
      mpu = MPU9250();
      MPU9250Setting settings;
      settings.accel_fs_sel = ACCEL_FS_SEL::A4G;
      settings.gyro_fs_sel = GYRO_FS_SEL::G500DPS;
      if (!mpu.setup(addr, settings, Wire)) continue; // Also checks AK8963.
      mpu.setMagneticDeclination(0.0f); // Magnetic north, no location assumption.
      _imuDetected = true;
      _imuType = RBIMUType::MPU9250;
      _imuModel = "MPU9250 + AK8963";
      _imuAddress = addr;
      return true;
    }
  }
  if (type == RBIMUType::Auto || type == RBIMUType::MPU6050) {
    for (uint8_t addr : {uint8_t(0x68), uint8_t(0x69)}) {
      if (address && address != addr) continue;
      if (!setupMPU6050(addr)) continue;
      _imuDetected = true;
      _imuType = RBIMUType::MPU6050;
      _imuModel = "MPU6050 (6-axis)";
      _imuAddress = addr;
      return true;
    }
  }
  return false;
}

bool RBNexusBoard::imuUpdate() {
  if (!_imuDetected) return false;
  if (_imuType == RBIMUType::MPU6050) {
    uint8_t status = 0, raw[14];
    if (!readIMURegisters(_imuAddress, 0x3A, &status, 1) || !(status & 1)) return false;
    if (!readIMURegisters(_imuAddress, 0x3B, raw, sizeof(raw))) return false;
    for (int i = 0; i < 3; ++i) {
      int16_t a = (int16_t)((uint16_t(raw[i*2]) << 8) | raw[i*2+1]);
      int16_t g = (int16_t)((uint16_t(raw[8+i*2]) << 8) | raw[9+i*2]);
      _accel[i] = a * (9.80665f / 8192.0f);
      _gyro[i] = g * (DEG_TO_RAD / 65.5f);
    }
    _imuReports = ACCEL_VALID | GYRO_VALID; // No magnetometer or orientation fusion.
    _imuAccelTime = _imuGyroTime = millis();
    return true;
  }
  if (_imuType == RBIMUType::MPU9250) {
    if (!probe(_imuAddress)) return false;
    if (!mpu.update()) return false;
    for (int i = 0; i < 3; ++i) {
      _accel[i] = mpu.getAcc(i) * 9.80665f;
      _gyro[i] = mpu.getGyro(i) * DEG_TO_RAD;
      _mag[i] = mpu.getMag(i) * 0.1f; // milligauss -> microtesla
    }
    _angles[0] = mpu.getRoll();
    _angles[1] = mpu.getPitch();
    _angles[2] = mpu.getYaw();
    _quaternion[0] = mpu.getQuaternionW();
    _quaternion[1] = mpu.getQuaternionX();
    _quaternion[2] = mpu.getQuaternionY();
    _quaternion[3] = mpu.getQuaternionZ();
    _imuReports = ACCEL_VALID | GYRO_VALID | ORIENTATION_VALID;
    _imuAccelTime = _imuGyroTime = _imuOrientationTime = millis();
    return true;
  }

  if (bno.wasReset()) {
    _imuReports = 0;
    if (!enableBNOReports()) { _imuDetected = false; return false; }
  }
  bool changed = false;
  // Bound work so servicing IMU does not starve motor/watchdog updates.
  for (uint8_t n = 0; n < 8; ++n) {
    sh2_SensorValue_t event = {};
    if (!bno.getSensorEvent(&event)) break;
    changed = true;
    uint32_t now = millis();
    switch (event.sensorId) {
      case SH2_ACCELEROMETER:
        _accel[0] = event.un.accelerometer.x;
        _accel[1] = event.un.accelerometer.y;
        _accel[2] = event.un.accelerometer.z;
        _imuAccelTime = now; _imuReports |= ACCEL_VALID;
        break;
      case SH2_GYROSCOPE_CALIBRATED:
        _gyro[0] = event.un.gyroscope.x;
        _gyro[1] = event.un.gyroscope.y;
        _gyro[2] = event.un.gyroscope.z;
        _imuGyroTime = now; _imuReports |= GYRO_VALID;
        break;
      case SH2_MAGNETIC_FIELD_CALIBRATED:
        _mag[0] = event.un.magneticField.x;
        _mag[1] = event.un.magneticField.y;
        _mag[2] = event.un.magneticField.z;
        break;
      case SH2_ROTATION_VECTOR: {
        float w = event.un.rotationVector.real, x = event.un.rotationVector.i;
        float y = event.un.rotationVector.j, z = event.un.rotationVector.k;
        float norm = sqrtf(w*w + x*x + y*y + z*z);
        if (!isfinite(norm) || norm < 0.001f) break;
        w /= norm; x /= norm; y /= norm; z /= norm;
        _quaternion[0] = w; _quaternion[1] = x; _quaternion[2] = y; _quaternion[3] = z;
        _angles[0] = atan2f(2*(w*x+y*z), 1-2*(x*x+y*y)) * RAD_TO_DEG;
        _angles[1] = asinf(constrain(2*(w*y-z*x), -1.0f, 1.0f)) * RAD_TO_DEG;
        _angles[2] = atan2f(2*(w*z+x*y), 1-2*(y*y+z*z)) * RAD_TO_DEG;
        _imuOrientationTime = now; _imuReports |= ORIENTATION_VALID;
        break;
      }
      default: break;
    }
  }
  return changed;
}

bool RBNexusBoard::imuDataFresh(uint32_t maxAgeMs) const {
  return _imuDetected && (_imuReports & 3) == 3 &&
         uint32_t(millis() - _imuAccelTime) <= maxAgeMs &&
         uint32_t(millis() - _imuGyroTime) <= maxAgeMs;
}
bool RBNexusBoard::imuOrientationFresh(uint32_t maxAgeMs) const {
  return _imuDetected && (_imuReports & ORIENTATION_VALID) &&
         uint32_t(millis() - _imuOrientationTime) <= maxAgeMs;
}
float RBNexusBoard::accelX() { return _accel[0]; }
float RBNexusBoard::accelY() { return _accel[1]; }
float RBNexusBoard::accelZ() { return _accel[2]; }
float RBNexusBoard::gyroX() { return _gyro[0]; }
float RBNexusBoard::gyroY() { return _gyro[1]; }
float RBNexusBoard::gyroZ() { return _gyro[2]; }
float RBNexusBoard::magX() { return _mag[0]; }
float RBNexusBoard::magY() { return _mag[1]; }
float RBNexusBoard::magZ() { return _mag[2]; }
float RBNexusBoard::roll() { return _angles[0]; }
float RBNexusBoard::pitch() { return _angles[1]; }
float RBNexusBoard::yaw() { return _angles[2]; }

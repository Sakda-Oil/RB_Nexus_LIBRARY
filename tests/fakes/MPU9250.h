#pragma once
enum class ACCEL_FS_SEL { A4G };
enum class GYRO_FS_SEL { G500DPS };
struct MPU9250Setting { ACCEL_FS_SEL accel_fs_sel; GYRO_FS_SEL gyro_fs_sel; };
inline bool fakeMPUNew = false;
class MPU9250 {
public:
  bool setup(uint8_t, MPU9250Setting, FakeWire&) { return true; }
  void setMagneticDeclination(float) {}
  bool update() { bool ready = fakeMPUNew; fakeMPUNew=false; return ready; }
  float getAcc(int i) { return i == 2 ? 1 : 0; }
  float getGyro(int i) { return i == 0 ? 180 : 0; }
  float getMag(int i) { return i == 1 ? 100 : 0; }
  float getRoll() { return 0; }
  float getPitch() { return 0; }
  float getYaw() { return 0; }
  float getQuaternionW() { return 1; }
  float getQuaternionX() { return 0; }
  float getQuaternionY() { return 0; }
  float getQuaternionZ() { return 0; }
};

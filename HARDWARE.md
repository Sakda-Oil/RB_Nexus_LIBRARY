# RB_Nexus hardware revision 0.1

Hardware information and verified peripheral pin mapping for **RB Nexus V0.1**:

| Item | Specification | Hardware Mapping & Verified Pins |
| --- | --- | --- |
| MCU | ESP32-WROOM-32, Wi-Fi / Bluetooth | 240 MHz, Dual Core, 4MB Flash |
| Board Status LED | LED State Indicator | **GPIO 2** (Active HIGH) - QC Pass |
| Board supply input | DC 6-24 V | Powers MCU buck converter, motors and PCA9685 |
| Digital I/O | 5 Channels (Input / Output) | **GPIO 4, 12, 14, 26, 27** - QC Pass |
| Quadrature Encoders | 4 Channels (Hall sensor inputs) | **ENC 1**: A=GPIO 36 (SP), B=GPIO 39 (SN)<br>**ENC 2**: A=GPIO 34, B=GPIO 35<br>**ENC 3**: A=GPIO 32, B=GPIO 33<br>**ENC 4**: A=GPIO 25, B=GPIO 13<br>- QC Pass |
| Brushed DC motor outputs | 4 Channels (H-Bridge via PCA9685) | Controlled via **PCA9685 I2C (Address 0x40)**:<br>**M1**: CH0, CH1<br>**M2**: CH2, CH3<br>**M3**: CH4, CH5<br>**M4**: CH6, CH7 |
| PWM / servo outputs | 8 Channels (labelled 8-15) | Controlled via **PCA9685 I2C (Address 0x40)**:<br>**CH8 - CH15**: 50 Hz PWM Servo outputs - QC Pass |
| Analog inputs | 8 Channels (labelled CH1-CH8) | Controlled via **MCP3208 12-bit SPI ADC**:<br>SPI: **MOSI=GPIO 23, MISO=GPIO 19, CLK=GPIO 18, CS=GPIO 5**<br>CH1 -> MCP3208 CH0<br>CH2 -> MCP3208 CH1<br>CH3 -> MCP3208 CH2<br>CH4 -> MCP3208 CH3<br>CH5 -> MCP3208 CH4<br>CH6 -> MCP3208 CH5<br>CH7 -> MCP3208 CH6<br>CH8 -> MCP3208 CH7 |
| I2C Bus | External I2C & PCA9685 | **SDA = GPIO 21, SCL = GPIO 22** |
| CAN Bus (TWAI) | External CAN transceiver | **TX = GPIO 17, RX = GPIO 16**; schematic checked, physical bus test required |

## Notes on Peripheral Support & Hardware QC

- **DC Motor Channels**:
  - The DC motor driver uses PCA9685 PWM outputs in pairs to drive H-bridges.
  - Forward: CH_A = PWM, CH_B = 0.
  - Reverse: CH_A = 0, CH_B = PWM.
  - Stop/Coast: CH_A = 0, CH_B = 0.
  - Brake: CH_A = 4095, CH_B = 4095.
  - Use `MotorTest.ino` (Diagnostic Mode) to test individual PCA9685 outputs (CH0 to CH7) to verify gate/driver signals.

- **Encoders**:
  - GPIO 34, 35, 36, 39 are input-only pins on ESP32 without internal pull-ups.
  - Open-collector Hall outputs require external pull-ups to 3.3 V; confirm the encoder module provides them or add them. Pull-ups are not shown for these inputs in the reviewed schematic.
  - GPIO 13, 25, 32, 33 utilize internal pull-ups in software.

- **MCP3208 ADC**:
  - Connected via standard ESP32 VSPI bus.
  - Full-scale reading: 0 to 4095 corresponding to 0 to 3.3V reference.

## External IMU support in software 0.2.2

- MPU6050: I2C 0x68/0x69, six-axis acceleration/gyro; no magnetometer.
- MPU6500: I2C 0x68/0x69, WHO_AM_I 0x70; six-axis acceleration/gyro, separate accelerometer DLPF.
- MPU9250 + AK8963: I2C 0x68/0x69 (magnetometer 0x0C).
- GY-BNO085: I2C 0x4A/0x4B in I2C mode.
- Shared SDA GPIO21, SCL GPIO22; I2C logic/pull-ups must be 3.3 V.
- Verify the supply and mode straps against the specific module.
- Historical QC statements above are not new physical test results for this software release.
- Relative Z rotation integrates gyro data in degrees on all supported IMUs. Keep Z vertical for planar turn measurements, calibrate while stationary, and expect drift; this is not absolute heading.

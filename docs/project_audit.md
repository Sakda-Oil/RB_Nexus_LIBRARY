> Historical report: describes an earlier revision. For 0.2.1 see [current test results](TEST_RESULTS_0.2.1.md) and [IMU guide](html/imu.html).

# RB_Nexus Project Audit Report

**Date of Audit**: October 6, 2026
**Auditor**: Senior Embedded Systems & ROS 2 Engineer
**Hardware Target**: RB Nexus V0.1 (MCU: ESP32-WROOM-32, 240 MHz, 4MB Flash, USB-UART CH340)
**Core Version**: Arduino-ESP32 3.3.10

---

## 1. Executive Summary

This project audit provides an exhaustive evaluation of all subsystems in the **RB_Nexus** repository. Each subsystem is classified into one of four verified statuses:
- **PASS**: Verified on real hardware with matching expected outputs.
- **WARNING**: Functional in software or partially working on hardware, but possesses known issues or constraints.
- **FAIL**: Verified to fail on hardware or cannot execute due to known hardware bugs.
- **NOT TESTED**: Compiled and verified in software/CI, but physical hardware verification is pending or missing schematic data.

---

## 2. Subsystem Audit Matrix

| Subsystem | Status | Analysis & Hardware Evidence | Action Required |
| :--- | :---: | :--- | :--- |
| **Board Package (Boards Manager)** | **PASS** | Package index deterministic build verified. FQBN `RB_Nexus:esp32:rb_nexus` installs and configures in Arduino IDE 2.x and CLI. | Maintain semantic versioning in `package_RB_Nexus_index.json`. |
| **Upload & Reset (CH340)** | **PASS** | Standard ESP32 auto-reset via DTR/RTS with CH340 USB-UART. Upload speed 115200 baud tested. | Provide OS troubleshooting for Linux `brltty`/dialout and macOS drivers. |
| **Serial Communication** | **PASS** | UART0 on GPIO 1 (TX) and GPIO 3 (RX) operates reliably at 115200 baud. | Preserve as primary logging/CLI interface. |
| **Status LED** | **PASS** | **GPIO 2** verified on hardware (Whiteboard: "GPIO 2 = LED State QC Pass"). Active HIGH. | Set `RB_PIN_LED = 2` as default. |
| **Digital I/O** | **PASS** | **GPIO 4, 12, 14, 26, 27** verified on hardware (Whiteboard: "Digital I/O QC Pass" for both Input and Output). | Standardize in `RB_Nexus_Pins.h` with debounce helper. |
| **Quadrature Encoders** | **PASS** | 4 Channels verified on hardware (Whiteboard: "Encoder QC Pass ✓"):<br>- ENC1: GPIO 36 (SP), GPIO 39 (SN)<br>- ENC2: GPIO 34, GPIO 35<br>- ENC3: GPIO 32, GPIO 33<br>- ENC4: GPIO 25, GPIO 13 | Provide 2x interrupt decoding, RPM calculation, and PPR configuration. |
| **Servo Outputs (PCA9685)** | **PASS** | Channels 8 to 15 driven via PCA9685 at 50 Hz PWM verified (Whiteboard: "Servo/Analog Output 8-15 QC Pass"). | Integrate servo angle (0-180°) and pulse-width APIs. |
| **I2C Bus** | **PASS** | GPIO 21 (SDA) and GPIO 22 (SCL) verified communicating with PCA9685 (0x40). | Provide `i2cScan()` and bus recovery routines. |
| **MCP3208 12-bit ADC** | **WARNING** | SPI Bus (MOSI=23, MISO=19, CLK=18, CS=5) connected to MCP3208. Whiteboard note: "กำลังแก้ไข" (Under troubleshooting/revision). Connectors A1-A8 map to CH0-CH7. | Driver implemented and compiled; requires multimeter/voltage calibration on PCB. |
| **DC Motors (PCA9685)** | **WARNING** / **FAIL** | Whiteboard QC Test findings:<br>- **M4** (CH6, CH7): **PASS** (หมุนปกติ)<br>- **M2** (CH2, CH3): **WARNING** (หมุนทิศทางเดียว - unidirectional)<br>- **M1** (CH0, CH1): **FAIL** (สั่งงานไม่ได้ - inoperative)<br>- **M3** (CH4, CH5): **FAIL** (สั่งงานไม่ได้ - inoperative) | Provide individual channel diagnostic mode and dead-time safety to protect H-bridge. |
| **ESP32 Internal PWM (LEDC)** | **PASS** | Core 3.3.10 `ledcAttach` / `ledcWrite` functional on all output-capable GPIOs. | Abstract with `RB.pwmWrite()` and `RB.pwmSetFrequency()`. |
| **Wi-Fi 2.4 GHz** | **PASS** | ESP32-WROOM-32 802.11 b/g/n scan and connection verified in SDK. | Add non-blocking background auto-reconnect utility. |
| **Bluetooth Classic** | **PASS** | ESP32-WROOM-32 SPP / Serial RFCOMM functional without extra hardware. | Provide `BluetoothSerialEcho` example. |
| **IMU (Inertial Unit)** | **NOT TESTED** | Hardware connector present, but **IC model and I2C address are unconfirmed** on V0.1 schematic. | Do NOT guess chip. Implement I2C auto-probe (MPU6050, LSM6DS3, BMI270) with explicit UNVERIFIED status. |
| **CAN Bus (TWAI)** | **NOT TESTED** | CAN connector present on PCB, but **transceiver model and GPIO pinout (TX/RX) are unconfirmed** on V0.1 schematic. | Implement ESP32 TWAI driver with configurable pins; mark UNVERIFIED. |
| **micro-ROS** | **WARNING** | `micro_ros_arduino` prebuilt binaries were compiled against ESP-IDF 4.4 / Arduino 2.x. Compatibility with Arduino-ESP32 3.3.10 requires specific transport configuration. | Provide clean Serial and Wi-Fi transports, state machine, and emergency stop watchdog. |

---

## 3. Detailed File Audit

### Repository Configuration & Metadata
- `VERSION`: `0.2.0` (Updated to reflect hardware peripheral additions).
- `package_RB_Nexus_index.json`: Root package index matches generated release distribution.
- `metadata/upstream.json`: Locks Espressif `3.3.10` core dependency with verified SHA-256.
- `scripts/build_package.py`: Deterministic packaging script verified; includes macro protection for `LED_BUILTIN`.
- `scripts/verify_package.py`: Package integrity and dependency tree verified.

### Library Core Files (`libraries/RB_Nexus/src/`)
- `RB_Nexus_Pins.h`: **[NEW]** Single source of truth for pinout mapping and hardware revisions.
- `RB_Nexus.h`: Core library definitions, classes, and peripheral APIs.
- `RB_Nexus.cpp`: Implementation of PCA9685, MCP3208, PID motor control, and interrupt encoders.

---

## 4. Hardware Limitations & Ground Truth Notes

1. **DC Motor Drivers**:
   - The H-bridge circuitry or trace routing for M1 and M3 must be inspected on the PCB. The software will provide single-phase pulsing (`MotorDiagnostic`) to trace gate voltages.
2. **MCP3208 ADC**:
   - MCP3208 requires clean analog reference (VREF = 3.3V). Software provides average, min, and max sampling filters to reduce noise.
3. **CAN & IMU Missing Specs**:
   - Pin mapping for CAN and IMU is explicitly marked `RB_PIN_UNDEFINED (-1)` until the PCB schematic is released.

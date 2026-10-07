// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file RB_Nexus_Pins.h
 * @brief Single Source of Truth for RB_Nexus Pin Definitions & Hardware Revisions.
 *
 * Pin mapping checked against Sheet_1 Rev 1.0 (2026-08-04), reviewed 2026-10-07.
 * Schematic agreement does not imply a physical hardware test.
 */

#pragma once
#include <Arduino.h>

// =============================================================================
// Hardware Revision Definition
// =============================================================================
#define RB_NEXUS_REV_01 1
#define RB_NEXUS_REV_02 2

#ifndef RB_NEXUS_CURRENT_REV
#define RB_NEXUS_CURRENT_REV RB_NEXUS_REV_01
#endif

// Defined constant for pins that are not connected or missing schematic data
#define RB_PIN_UNDEFINED -1

// =============================================================================
// RB Nexus Revision 0.1 Pin Mapping
// =============================================================================
#if (RB_NEXUS_CURRENT_REV == RB_NEXUS_REV_01)

// --- Status LED ---
constexpr int RB_PIN_LED = 2; // Active HIGH

// --- Digital I/O ---
constexpr int RB_PIN_D4  = 4;
constexpr int RB_PIN_D12 = 12;
constexpr int RB_PIN_D14 = 14;
constexpr int RB_PIN_D26 = 26;
constexpr int RB_PIN_D27 = 27;

// --- Quadrature Encoders (4 Channels) ---
// ENC1: Uses ESP32 input-only dedicated sensor pins
constexpr int RB_PIN_ENC1_A = 36; // SENSOR_VP (SP)
constexpr int RB_PIN_ENC1_B = 39; // SENSOR_VN (SN)

// ENC2: Uses ESP32 input-only pins
constexpr int RB_PIN_ENC2_A = 34;
constexpr int RB_PIN_ENC2_B = 35;

// ENC3: General purpose GPIOs
constexpr int RB_PIN_ENC3_A = 32;
constexpr int RB_PIN_ENC3_B = 33;

// ENC4: General purpose GPIOs
constexpr int RB_PIN_ENC4_A = 25;
constexpr int RB_PIN_ENC4_B = 13;

// --- I2C Bus (Connected to PCA9685 PW & External Connector) ---
constexpr int RB_PIN_I2C_SDA = 21;
constexpr int RB_PIN_I2C_SCL = 22;
constexpr uint8_t RB_PCA9685_ADDR = 0x40;

// --- SPI Bus (Connected to MCP3208 12-bit ADC) ---
constexpr int RB_PIN_SPI_MOSI = 23;
constexpr int RB_PIN_SPI_MISO = 19;
constexpr int RB_PIN_SPI_CLK  = 18;
constexpr int RB_PIN_SPI_CS   = 5;

// --- CAN Bus (TWAI) ---
// U12 GPIO17 (header pin 28) -> U24 D (pin 1).
// U12 GPIO16 (header pin 27) <- U24 R (pin 4).
// Use GPIO numbers here, not the NodeMCU header pin numbers.
constexpr int RB_PIN_CAN_TX = 17;
constexpr int RB_PIN_CAN_RX = 16;

// --- External IMU (I2C): MPU9250/MPU6050 0x68/0x69 or BNO085 0x4A/0x4B ---
constexpr uint8_t RB_IMU_I2C_ADDR = 0x00; // Auto-probe mode

// --- PCA9685 Channel Allocations ---
// Motors M1 - M4 (Channels 0 to 7)
constexpr uint8_t RB_MOTOR1_CH_A = 0;
constexpr uint8_t RB_MOTOR1_CH_B = 1;
constexpr uint8_t RB_MOTOR2_CH_A = 2;
constexpr uint8_t RB_MOTOR2_CH_B = 3;
constexpr uint8_t RB_MOTOR3_CH_A = 4;
constexpr uint8_t RB_MOTOR3_CH_B = 5;
constexpr uint8_t RB_MOTOR4_CH_A = 6;
constexpr uint8_t RB_MOTOR4_CH_B = 7;

// Servos (Channels 8 to 15)
constexpr uint8_t RB_SERVO_CH_MIN = 8;
constexpr uint8_t RB_SERVO_CH_MAX = 15;
constexpr uint8_t RB_SERVO_COUNT  = 8;

// --- MCP3208 Analog Channel Mapping (12-bit, 0-4095) ---
// Connector A1 -> MCP3208 CH0 ... A8 -> MCP3208 CH7
constexpr uint8_t RB_ADC_CH_MIN = 1;
constexpr uint8_t RB_ADC_CH_MAX = 8;
constexpr uint8_t RB_ADC_COUNT  = 8;

#endif // RB_NEXUS_REV_01

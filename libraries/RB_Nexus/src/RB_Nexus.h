// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <Arduino.h>
#if !defined(ARDUINO_ARCH_ESP32)
#error "RB_Nexus requires an ESP32 board selected in Tools > Board."
#endif
namespace RBNexus {
constexpr const char* name = "RB_Nexus";
constexpr const char* version = "0.1.0";
constexpr bool peripheralPinMapAvailable = false;
}

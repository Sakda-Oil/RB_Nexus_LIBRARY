// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "RB_Nexus.h"

// Feed only after a valid motor command, never unconditionally in loop().
class RBMotorCommandGuard {
public:
  explicit RBMotorCommandGuard(uint32_t timeoutMs = 500) : _timeout(timeoutMs) {}
  void feed() { _last = millis(); _received = true; }
  void update() {
    if (_received && uint32_t(millis() - _last) >= _timeout) {
      RB.motorStopAll();
      _received = false;
    }
  }
private:
  uint32_t _timeout, _last = 0;
  bool _received = false;
};

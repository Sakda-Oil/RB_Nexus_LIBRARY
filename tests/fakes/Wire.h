#pragma once
#include <stdint.h>
#include <vector>
struct FakeWire {
  uint8_t registers[256] = {};
  std::vector<uint8_t> tx;
  uint8_t pointer = 0;
  uint8_t remaining = 0;
  int shortRead = -1;
  bool begin(int, int, uint32_t) { return true; }
  void beginTransmission(uint8_t) { tx.clear(); }
  void write(uint8_t b) { tx.push_back(b); }
  void setTimeOut(uint16_t) {}
  int endTransmission(bool = true) {
    if (!tx.empty()) {
      pointer = tx[0];
      for (size_t i=1; i<tx.size(); ++i) registers[uint8_t(pointer+i-1)] = tx[i];
    }
    return 0;
  }
  uint8_t requestFrom(uint8_t, uint8_t count) {
    remaining = shortRead < 0 ? count : uint8_t(shortRead);
    return remaining;
  }
  bool available() { return remaining > 0; }
  uint8_t read() { if (remaining) --remaining; return registers[pointer++]; }
};
inline FakeWire Wire;

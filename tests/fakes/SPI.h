#pragma once
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings { SPISettings(uint32_t, int, int) {} };
struct FakeSPI {
  void begin(int,int,int,int) {}
  void beginTransaction(SPISettings) {}
  uint8_t transfer(uint8_t) { return 0; }
  void endTransaction() {}
};
inline FakeSPI SPI;

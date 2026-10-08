#pragma once
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings { SPISettings(uint32_t, int, int) {} };
inline uint16_t fakeADCValue = 0;
struct FakeSPI {
  uint8_t position = 0;
  void begin(int,int,int,int) {}
  void beginTransaction(SPISettings) { position = 0; }
  uint8_t transfer(uint8_t) {
    ++position;
    return position == 2 ? (fakeADCValue >> 8) : (position == 3 ? fakeADCValue & 255 : 0);
  }
  void endTransaction() {}
};
inline FakeSPI SPI;

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string>
#define IRAM_ATTR
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
#define CHANGE 3
#define DEG_TO_RAD 0.017453292519943295f
#define RAD_TO_DEG 57.29577951308232f
using String = std::string;
inline uint32_t fakeNow = 100;
inline int fakePins[40] = {};
inline uint32_t fakeFreq[40] = {}, fakeDuty[40] = {};
inline uint8_t fakeResolution[40] = {};
inline uint32_t millis() { return fakeNow; }
inline uint32_t micros() { return fakeNow * 1000u; }
inline void delay(uint32_t ms) { fakeNow += ms; }
inline void delayMicroseconds(uint32_t) {}
inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t pin) { return fakePins[pin]; }
inline void digitalWrite(uint8_t pin, uint8_t val) { fakePins[pin] = val; }
inline int digitalPinToInterrupt(int pin) { return pin; }
inline void attachInterrupt(int, void (*)(), int) {}
inline bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t bits) {
  if (fakeFreq[pin]) return false;
  fakeFreq[pin] = freq; fakeResolution[pin] = bits; return true;
}
inline bool ledcWrite(uint8_t pin, uint32_t duty) { fakeDuty[pin] = duty; return true; }
inline uint32_t ledcReadFreq(uint8_t pin) { return fakeFreq[pin]; }
inline uint32_t ledcChangeFrequency(uint8_t pin, uint32_t freq, uint8_t bits) {
  fakeFreq[pin] = freq; fakeResolution[pin] = bits; return freq;
}
inline bool ledcDetach(uint8_t pin) { fakeFreq[pin] = 0; return true; }
class Print {
public:
  template<typename... T> void printf(const char*, T...) {}
  template<typename T> void print(T) {}
  template<typename T> void println(T) {}
  void println() {}
};
inline Print Serial;
template<typename T> T constrain(T x, T lo, T hi) { return x < lo ? lo : (x > hi ? hi : x); }

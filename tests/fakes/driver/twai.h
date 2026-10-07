#pragma once
#define TWAI_MODE_NORMAL 0
#define ESP_OK 0
#define TWAI_GENERAL_CONFIG_DEFAULT(tx,rx,mode) {tx,rx,mode}
#define TWAI_TIMING_CONFIG_125KBITS() {}
#define TWAI_TIMING_CONFIG_250KBITS() {}
#define TWAI_TIMING_CONFIG_500KBITS() {}
#define TWAI_TIMING_CONFIG_1MBITS() {}
#define TWAI_FILTER_CONFIG_ACCEPT_ALL() {}
#define pdMS_TO_TICKS(ms) (ms)
struct twai_general_config_t { int tx_io; int rx_io; int mode; };
struct twai_timing_config_t {};
struct twai_filter_config_t {};
struct twai_message_t {
  uint32_t identifier;
  unsigned extd:1, rtr:1, ss:1, self:1, dlc_non_comp:1;
  uint8_t data_length_code, data[8];
};
inline twai_message_t fakeSent;
inline twai_general_config_t fakeInstalledCAN;
inline unsigned fakeCANInstallCount = 0;
inline int twai_driver_install(const twai_general_config_t* config,void*,void*) {
  fakeInstalledCAN = *config;
  ++fakeCANInstallCount;
  return 0;
}
inline int twai_start() { return 0; }
inline int twai_stop() { return 0; }
inline int twai_driver_uninstall() { return 0; }
inline int twai_transmit(twai_message_t* msg, int) { fakeSent=*msg; return 0; }
inline int twai_receive(twai_message_t*, int) { return -1; }

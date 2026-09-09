# RB_Nexus hardware revision 0.1

Hardware information supplied in the RB Nexus 0.1 specification:

| Item | Specification |
| --- | --- |
| MCU | ESP32-WROOM-32, Wi-Fi / Bluetooth |
| Board supply input | DC 6-24 V |
| Brushed DC motor outputs | 4 channels, 6-24 V, stated 7 A continuous / 15 A maximum |
| Encoder inputs | 4 Hall sensor encoder channels |
| Analog inputs | 8 channels labelled 1-8, stated 12-bit range 0-4095 |
| PWM / servo outputs | 8 channels labelled 8-15 |
| Digital I/O ADC | Labels 4, 12, 14, 26, 27 |
| Communication | CAN terminal, external I2C JST-XH 4P, IMU connector |

## Information still needed for peripheral support

- Exact GPIO mapping, driver chips and logic truth tables for each motor.
- Encoder signal A/B GPIOs, supply levels and pull-ups.
- ADC chip, bus and channel mapping for analogue inputs.
- PWM/servo chip, bus and channel mapping for outputs.
- CAN transceiver/controller model and TX/RX/CS/interrupt pins.
- I2C SDA/SCL, IMU model/address, indicator LED pin and polarity.
- Flash capacity and USB-to-UART chip/automatic-reset wiring.

Connector numbers are not assumed to be ESP32 GPIO numbers. Version 0.1.0 does
not define motor(), encoder(), servo() or analogue channel functions.

The pin variant preserves the standard ESP32 Dev Module aliases (SDA=21,
SCL=22, TX=1, RX=3, standard SPI and ADC aliases). These are generic defaults,
not a verified map of the RB_Nexus connectors. Pass actual pins to bus setup
functions after checking the schematic. No LED_BUILTIN pin is invented.

4 MB flash, DIO 40 MHz and PSRAM disabled are initial software defaults, not
hardware measurements. Match Tools settings to the actual module. The 6-24 V
rating describes the board power input, not the ESP32 GPIO voltage.

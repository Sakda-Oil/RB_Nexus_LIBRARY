# Source and changes

RB_Nexus package 0.1.1 is maintained by Sakda-Oil.

It redistributes the Arduino-ESP32 3.3.10 platform from Espressif Systems:
https://github.com/espressif/arduino-esp32/tree/3.3.10

The upstream core sources and library sources are included in the platform ZIP.
Copyright and license notices inside upstream files remain intact. The upstream
LGPL-2.1 license text is included in LICENSE.md; individual components retain
their own licenses. RB_Nexus additions use LGPL-2.1-or-later.

Changes dated 2026-09-09:
- Replace the board list with RB_Nexus, derived from ESP32 Dev Module.
- Rename platform and set RB_Nexus package version to 0.1.1.
- Add a generic ESP32 pin variant with an explicit pending-peripheral-map notice.
- Display both platform and board as RB_Nexus.
- Add SerialEcho, WiFiScan, DigitalInput, AnalogInput, PWMOutput and I2CScanner examples.
- Default serial upload speed to 115200 baud.
- Add RB_Nexus identification header, examples and documentation.

Compiler, SDK and utility archives are downloaded from the original upstream
URLs recorded in metadata/upstream.json. Their SHA-256 hashes and platform
availability are preserved. Tool definitions are registered under RB_Nexus so
installation needs only the RB_Nexus index URL.

No source code or branding has been copied from FRIENDROBOT_LIBRARY.
That repository was used only as a reference for the installation approach.

# RB_Nexus 0.2.1

- I2C IMU support for MPU6050, MPU9250 + AK8963 and GY-BNO085 with SI units and sample freshness checks.
- Fixed PID stop behavior, shared RB/RB_Nexus state, motor speed saturation, servo timing and repeated GPIO PWM writes.
- Real micro-ROS types/transports, valid IMU messages, initialization error handling and 500 ms motor-command timeout.
- Updated Thai HTML installation, API, IMU, ROS and testing guides.
- Regression tests run against the production driver with fake hardware. CI compiles examples with actual IMU and micro-ROS dependencies.

Install MPU9250 0.4.8 and Adafruit BNO08x 1.2.7 with their dependencies using Arduino Library Manager. micro-ROS examples additionally require the Jazzy library described in the HTML guide.

Physical board testing, IMU calibration/axis validation and ROS Agent runtime testing remain required. This release does not claim to repair V0.1 PCB defects reported for motors M1/M2/M3.

# RB_Nexus 0.2.5

Arduino board package และไลบรารีสำหรับ RB Nexus V0.1 (ESP32-WROOM-32) บน Arduino-ESP32 core 3.3.10

รองรับ PCA9685 มอเตอร์/Servo, MCP3208, Encoder, **MPU6050, MPU6500, MPU9250 + AK8963 และ GY-BNO085 ผ่าน I2C** พร้อมตัวอย่าง micro-ROS ที่ใช้ไลบรารีจริง

## ติดตั้ง

เพิ่ม URL ใน Arduino IDE → Preferences → Additional Boards Manager URLs:

```text
https://raw.githubusercontent.com/Sakda-Oil/RB_Nexus_LIBRARY/main/package_RB_Nexus_index.json
```

ติดตั้งบอร์ด RB_Nexus จาก Boards Manager รุ่น 0.2.2 ขึ้นไปมีรายการไลบรารีที่ต้องใช้ในตัว: **MPU9250 by hideakitai 0.4.8**, **Adafruit BNO08x 1.2.7**, **Adafruit BusIO 1.17.4**, **Adafruit Unified Sensor 1.1.15**

Arduino IDE 2.4.0 / Arduino CLI 1.5.0 ขึ้นไปรองรับการติดตั้งไลบรารีเหล่านี้พร้อมบอร์ด หากมีรุ่นเท่ากับหรือใหม่กว่าที่กำหนดแล้วจะเก็บรุ่นเดิมไว้ ตาม [ข้อกำหนด Arduino](https://docs.arduino.cc/arduino-cli/package_index_json-specification/#platforms-definitions) สำหรับ IDE รุ่นเก่าให้ติดตั้งผ่าน Library Manager และเลือก Install All; ต้องมีทั้ง MPU9250 และ BNO08x แม้ใช้ MPU6500 ตัวเดียว หากไฟล์หลักขาด โค้ดจะแจ้งชื่อไลบรารีที่ต้องติดตั้งตอน Verify/Upload

ตรวจไลบรารีและไฟล์หลักโดยไม่ติดตั้งเพิ่มด้วย `python scripts/install_dependencies.py --check-only --skip-microros` ถ้าขาดหรือรุ่นเก่า โปรแกรมจะแจ้งรายการและจบด้วยรหัส 1 ตัด `--check-only` ออกเพื่อติดตั้งรุ่นที่ใช้ทดสอบ ต้องติดตั้ง Arduino CLI และใช้ `--config-file` ให้ตรงกับโฟลเดอร์ Sketchbook ของ IDE; ผลตรวจของโปรไฟล์ทดสอบไม่ได้ยืนยันโปรไฟล์ IDE อีกชุดหนึ่ง

ตัวอย่าง ROS ต้องใช้ micro_ros_arduino เพิ่มด้วย: เรียกตัวติดตั้งโดยไม่ใส่ `--skip-microros` เพื่อใช้ Jazzy snapshot ที่โครงการทดสอบ ไม่รวม micro-ROS ในรายการติดตั้งอัตโนมัติของบอร์ด เพราะต้องใช้ snapshot เฉพาะ

## อ่าน IMU

SDA=GPIO21, SCL=GPIO22, GND ร่วม; สัญญาณ I2C 3.3 V ใช้ไฟเลี้ยงตามสเปกโมดูลจริง

```cpp
#include <RB_Nexus.h>

void setup() {
  Serial.begin(115200);
  RB.begin();
  // เลือก Auto, MPU6050, MPU6500, MPU9250 หรือ BNO085 (ครั้งละหนึ่งตัว)
  if (!RB.imuBegin(RBIMUType::Auto)) Serial.println("IMU initialization failed; check wiring and WHO_AM_I");
  else Serial.println(RB.imuName());
}
void loop() {
  RB.update();
  static uint32_t last = 0;
  if (millis() - last >= 100) {
    last = millis();
    if (RB.imuFresh()) {
      Serial.printf("a[m/s^2] %.3f %.3f %.3f  gyro[rad/s] %.3f %.3f %.3f\n",
        RB.accelX(), RB.accelY(), RB.accelZ(), RB.gyroX(), RB.gyroY(), RB.gyroZ());
    } else Serial.println("Waiting for fresh IMU data");
  }
}
```

Auto เลือก BNO085 ก่อน MPU9250/MPU9255 แล้วจึง MPU6050/MPU6500; ใช้หนึ่งเซนเซอร์ในแต่ละครั้ง เลือกชนิด/address เองได้ผ่าน imuBegin(type, address) ค่า WHO_AM_I=0x70 จะเลือกไดรเวอร์ MPU6500 และรายงานชื่อจริง แม้โมดูลจะติดป้าย MPU9250; การตอบ ACK หรือ WHO_AM_I ไม่ใช่หลักฐานยืนยันของแท้
ค่าความเร่ง m/s² รวม gravity, gyro rad/s, magnetic field µT และ Euler องศา ต้องตรวจ imuFresh() ก่อนใช้ข้อมูล

MPU6050/MPU6500 รองรับ accel/gyro 6 แกน ไม่มี magnetometer และยังไม่ทำ orientation fusion: Euler/quaternion เป็น NaN และ imuOrientationFresh() เป็น false

## วัดองศาหมุนรอบแกน Z

เปิด File → Examples → RB_Nexus → **IMURotationZ** อัปโหลดแล้วเปิด Serial Monitor 115200 สำหรับ MPU ให้วางเซนเซอร์นิ่ง 3 วินาทีเพื่อหาค่าชดเชย ส่วน BNO085 ใช้ calibrated gyro ของเซนเซอร์โดยไม่บังคับคาลิเบรตซ้ำแบบ MPU จากนั้นหมุนรอบแกน Z จะเห็นมุมสะสม และส่งตัวเลข `0` เพื่อตั้งศูนย์ใหม่

```cpp
RB.update();
if (RB.headingFresh()) {
  Serial.println(RB.heading());   // องศาสะสม มีเครื่องหมาย ไม่จำกัด 360
}
// RB.resetHeading();             // ตั้งจุดอ้างอิงใหม่ รอ gyro ตัวอย่างถัดไป
// RB.setHeadingBias(biasRadS);   // ค่าเฉลี่ย gyroZ() ขณะวางนิ่ง หน่วย rad/s
```

ตั้งแต่ 0.2.3 **หมุนขวา/ตามเข็มนาฬิกาเป็นบวก หมุนซ้ายเป็นลบ เมื่อมองจากด้านบนและแกน +Z ของเซนเซอร์ชี้ขึ้น** การเปลี่ยนทิศใช้เฉพาะ heading(); gyroZ(), yaw(), quaternion และข้อมูล ROS ยังใช้กรอบแกนเดิม ค่ามุม Z นี้คำนวณจาก gyro จึงสะสมความคลาดเคลื่อนได้กับทุกเซนเซอร์และไม่ใช่มุมเทียบทิศเหนือ เรียก update ต่อเนื่องทุก 5–20 ms; ก่อนมีข้อมูลจะได้ NaN และช่วงข้อมูลหายเกิน 250 ms จะไม่ประมาณมุมที่ขาดหาย ต้องตั้งศูนย์ใหม่หากเคลื่อนที่ระหว่างช่วงนั้น ความสดของข้อมูลไม่ได้รับรองความแม่นยำของมุม สำหรับ MPU9250/BNO085 ยังอ่านมุมจาก sensor fusion ได้ทาง yaw() โดยตรวจ imuOrientationFresh()

หาก BNO085 ขึ้นชื่อรุ่นแล้วแต่ไม่มีค่า ให้เปิดตัวอย่าง **BNO085Diagnostic** ซึ่งเลือก BNO085 โดยตรงและแสดงจำนวน gyro samples, ค่าดิบ และสถานะความสด รุ่น 0.2.3 รับรายงานผ่าน SH2 callback ทุกค่าในแพ็กเกจเดียว แก้การสูญเสีย gyro เมื่อรายงาน rotation vector ตามมาในแพ็กเกจเดียวกัน `imuSampleCount()` ใช้นับตัวอย่างใหม่ โดยเริ่มนับใหม่เมื่อ imuBegin()

รุ่น **0.2.5** แก้อาการ BNO085 ตรวจพบและมี gyro samples แต่มุม Z เพิ่มน้อยหรือค้าง: ไดรเวอร์ I2C ของ Adafruit 1.2.7 ไม่เติมเวลาให้แพ็กเกจ จึงต้องเติมเวลาฝั่ง ESP32 ก่อนส่งเข้า SH2 และรักษาเวลาย่อยของรายงานในแพ็กเกจ อัปเดตบอร์ดใน Boards Manager แล้วอัปโหลด IMURotationZ ใหม่ ตรวจบรรทัดแรกว่าเป็น 0.2.5 การติดตั้งบอร์ดอย่างเดียวไม่เปลี่ยนโปรแกรมที่อยู่บนบอร์ดแล้ว เรียก RB.update() ต่อเนื่องเพื่อจำกัดความคลาดเคลื่อนของเวลา polling หากยังมีปัญหา ใช้ BNO085Diagnostic ดู gyro XYZ (rad/s), มุมสะสม Z และ fused yaw (องศา) ซึ่งใช้กรอบแกนและจุดอ้างอิงต่างกัน

หาก Serial Monitor ขึ้น `Loading index file ... RB_Nexus_LIBRARY.git` ให้ลบ URL ที่ลงท้าย `.git` ออกจาก Additional Boards Manager URLs แล้วใช้ URL `package_RB_Nexus_index.json` ด้านบน จากนั้นเปิด IDE/เครื่องมือ Serial Monitor ใหม่ ข้อผิดพลาดดัชนีในคอมพิวเตอร์เป็นคนละส่วนกับข้อมูล IMU ที่ส่งจากบอร์ด

## คำสั่งทั้งหมด

ทุกคำสั่งมีชื่อเดียว เลขช่องเริ่มที่ 1 เสมอ (มอเตอร์ 1–4, เซอร์โว 1–8, อนาล็อก 1–8, encoder 1–4)
และกำลังมอเตอร์เป็นเปอร์เซ็นต์ −100..100 ทุกคำสั่ง คำสั่งที่ใช้บ่อยใช้ชื่อสั้น ส่วนคำสั่งเฉพาะอุปกรณ์
ขึ้นต้นด้วยชื่ออุปกรณ์ (`motor…`, `servo…`, `encoder…`, `analog…`, `pin…`, `imu…`)

| หมวด | คำสั่ง |
|---|---|
| ระบบ | `RB.begin()` · `RB.update()` (ทุก loop) · `RB.wait(ms)` แทน `delay()` |
| LED | `RB.led(true/false)` · `RB.ledToggle()` |
| ขับหุ่น 2 ล้อ | `RB.setWheels(ซ้าย, ขวา)` · `RB.reverseMotor(id)` · `RB.forward(%, ms)` · `RB.backward(%, ms)` · `RB.turnLeft(%, ms)` · `RB.turnRight(%, ms)` · `RB.drive(ซ้าย%, ขวา%)` · `RB.turnDegrees(องศา)` · `RB.stop()` · `RB.brake()` |
| มอเตอร์ทีละตัว | `RB.motorPower(id, %)` · `RB.motorRPM(id, rpm)` · `RB.motorStop(id)` · `RB.motorBrake(id)` · `RB.motorPID(id, kp, ki, kd)` · `RB.motorPIDActive(id)` · `RB.motorDeadTime(ms)` |
| เซอร์โว 1–8 | `RB.servoAngle(n, องศา)` · `RB.servoPulse(n, us)` · `RB.servoRelease(n)` |
| Encoder 1–4 | `RB.encoderBegin()` · `RB.encoderTicks(n)` · `RB.encoderRPM(n)` · `RB.encoderReset(n)` · `RB.encoderResetAll()` · `RB.encoderSetPPR(n, ticks)` |
| อนาล็อก A1–A8 | `RB.analogRaw(ch)` 0–4095 · `RB.analogVolt(ch)` · `RB.analogPercent(ch)` · `RB.analogAverage(ch, n)` |
| ขา GPIO | `RB.pinOn(pin)` · `RB.pinOff(pin)` · `RB.pinRead(pin)` · `RB.pinPWM(pin, %)` · `RB.buttonPressed(pin)` |
| IMU | `RB.imuBegin()` · `RB.imuReady()` · `RB.imuFresh()` · `RB.imuName()` · `RB.heading()` · `RB.resetHeading()` · `RB.headingFresh()` · `RB.setHeadingBias(rad/s)` · `RB.roll()/pitch()/yaw()` · `RB.accelX()…` · `RB.gyroX()…` · `RB.magX()…` |
| ความปลอดภัย | `RB.emergencyStop()` · `RB.emergencyClear()` · `RB.emergencyActive()` · `RB.watchdogStart(ms)` · `RB.watchdogFeed()` · `RB.watchdogStop()` |
| สื่อสาร | `RB.wifiConnect(ssid, pass)` · `RB.wifiConnected()` · `RB.i2cScan()` · `RB.canBegin()` · `RB.canSend()` · `RB.canReceive()` |
| ขั้นสูง (PCA9685) | `RB.pcaReady()` · `RB.pcaDuty(ch, 0–4095)` · `RB.pcaFrequency(hz)` (มอเตอร์และเซอร์โวใช้ความถี่ร่วมกัน เซอร์โวต้องการ 50 Hz) |

```cpp
void setup() {
  RB.begin();
  RB.imuBegin();
  RB.setWheels(1, 2);
  RB.reverseMotor(2);       // ล้อขวาต่อสายกลับด้าน
}
void loop() {
  RB.update();
  if (RB.buttonPressed(4)) {
    RB.forward(50, 1000);   // เดินหน้า 50% 1 วินาที แล้วหยุด
    RB.turnDegrees(90);     // เลี้ยวขวา 90° ด้วย IMU
    RB.servoAngle(1, 45);
  }
}
```

`RB.forward(…, ms)`, `RB.turnDegrees()` และ `RB.wait()` รอจนเสร็จ ระหว่างนั้นยังเรียก `RB.update()` ให้
`RB.turnDegrees()` คืน `false` และหยุดมอเตอร์ หากไม่มีข้อมูล IMU หรือหมุนไม่ถึงภายในเวลาที่กำหนด (ค่าเริ่มต้น 5 วินาที)
`RB.reverseMotor()` มีผลกับคำสั่งกำลัง (`motorPower`, `forward`, `drive` ฯลฯ) ไม่กลับทิศ PID/encoder
`RB.emergencyStop()` หยุดมอเตอร์และล็อกไว้จนเรียก `RB.emergencyClear()`

### เปลี่ยนชื่อจากรุ่น 0.2.5

ชื่อซ้ำซ้อนถูกเอาออก โปรแกรมเดิมต้องแก้ตามตารางนี้

| ชื่อเดิม | ชื่อใหม่ |
|---|---|
| `setLED`, `toggleLED` | `led`, `ledToggle` |
| `motorSet`, `motor` (±255) | `motorPower` (±100 %) |
| `motorSetPercent`, `motorPercent` | `motorPower` |
| `stop(id)` | `motorStop(id)` |
| `motorStopAll`, `stopAll` | `stop()` |
| `motorSetRPM`, `motorSetPID`, `motorPIDEnable/Disable`, `isMotorPIDEnabled` | `motorRPM`, `motorPID`, (`motorRPM`/`motorStop`), `motorPIDActive` |
| `servo(8..15, a)`, `servoWrite`, `servoAttach` | `servoAngle(1..8, a)` |
| `servoWriteMicroseconds`, `servoMicroseconds`, `servoDetach` | `servoPulse(1..8, us)`, `servoRelease` |
| `encoderRead`, `readEncoder`, `resetEncoder`, `resetAllEncoders` | `encoderTicks`, `encoderReset`, `encoderResetAll` |
| `encoderDirection` | ดูเครื่องหมายของ `encoderRPM` |
| `analogRead`, `readAnalog`, `analogVoltage`, `readAnalogVoltage`, `analogReadAverage` | `analogRaw`, `analogVolt`, `analogAverage` |
| `pinMode`, `digitalWrite`, `digitalRead` ของ RB | `pinOn`, `pinOff`, `pinRead` (หรือฟังก์ชัน Arduino ปกติ) |
| `pwmWrite`, `pwmSetFrequency`, `pwmStop` | `pinPWM(pin, %, Hz)`, `pinOff` |
| `setPWM`, `setPWMDuty`, `setPWMFreq`, `isPCA9685Connected` | `pcaDuty`, `pcaFrequency`, `pcaReady` |
| `imuDataFresh`, `isIMUAvailable`, `imuModelName`, `imuGyroSampleCount` | `imuFresh`, `imuReady`, `imuName`, `imuSampleCount` |
| `imuRotationZ`, `angleZ`, `imuRotationZFresh` | `heading`, `headingFresh` |
| `imuResetRotationZ`, `zeroZ`, `imuSetGyroZBias` | `resetHeading`, `setHeadingBias` |
| `isEmergencyStopped`, `clearEmergencyStop` | `emergencyActive`, `emergencyClear` |
| `enableWatchdog`, `disableWatchdog`, `feedWatchdog` | `watchdogStart`, `watchdogStop`, `watchdogFeed` |
| `wifiEnableAutoReconnect`, `i2cBegin` | `wifiAutoReconnect`, (`RB.begin(i2cFreq)`) |
| `RB_Nexus` (อ็อบเจกต์) | `RB` |
| เซนเซอร์: `readCentimeters`, `readVoltage`, `isObstacleDetected` | `cm`, `volts`, `detected` |

## micro-ROS

`RBROS` เชื่อมต่อ agent ให้อัตโนมัติ เปิด agent ก่อนหรือหลังบอร์ดก็ได้ หาก agent หลุด
มอเตอร์ทุกตัวหยุดทันที แล้วบอร์ดรอเชื่อมต่อใหม่เอง ไม่ต้องกดรีเซ็ต

```cpp
#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t counterPub;
std_msgs__msg__Int32 counter;

bool createROS() {          // สร้าง publisher/subscriber ทุกครั้งที่เชื่อมต่อ
  return RBROS.publisher(counterPub, RB_MSG(std_msgs, Int32), "/rb_nexus/counter");
}
void setup() {
  RB.begin();
  RBROS.begin("rb_nexus_publisher", createROS);   // USB Serial 115200
}
void loop() {
  RB.update();
  RBROS.spin();
  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 1000) {
    last = millis();
    counter.data++;
    RBROS.publish(counterPub, &counter);
  }
}
```

| คำสั่ง | ทำอะไร |
|---|---|
| `RBROS.useSerial(baud)` / `RBROS.useWiFi(ssid, pass, agentIP, port)` | เลือกช่องทาง (ค่าเริ่มต้น USB Serial 115200) |
| `RBROS.begin(ชื่อnode, createROS)` | เริ่มทำงาน ไม่บล็อก |
| `RBROS.spin()` | เรียกทุก loop: เชื่อมต่อ/เชื่อมต่อใหม่/รับข้อความ |
| `RBROS.connected()` · `RBROS.state()` · `RBROS.stateText()` | สถานะการเชื่อมต่อ |
| `RBROS.publisher(pub, RB_MSG(pkg, Type), topic)` | สร้าง publisher (สูงสุด 10) |
| `RBROS.subscriber(sub, RB_MSG(pkg, Type), topic, &msg, callback)` | สร้าง subscriber (สูงสุด 8) |
| `RBROS.publish(pub, &msg)` | ส่งเฉพาะเมื่อเชื่อมต่ออยู่ |
| `RBROS.fillImu(msg)` · `RBROS.stamp(header.stamp)` | เติมข้อมูล IMU / เวลาจาก agent |
| `RBROS.stopMotorsWhenLost(false)` | ปิดการหยุดมอเตอร์เมื่อ agent หลุด (ไม่แนะนำ) |

คำสั่งมอเตอร์ทาง ROS (`/rb_nexus/motor/1/cmd`, `std_msgs/Int16`) เป็นเปอร์เซ็นต์ −100..100
ส่งซ้ำอย่างน้อย 10 Hz เพราะ `RBMotorCommandGuard` หยุดมอเตอร์เมื่อไม่มีคำสั่ง 500 ms
ห้ามพิมพ์ข้อความลง `Serial` เมื่อใช้ USB Serial transport เพราะเป็นช่องเดียวกับข้อมูล ROS

## CAN Bus

ขาตามวงจร: **TX=GPIO17, RX=GPIO16** เริ่มใช้งานด้วย `RB.canBegin(500000)` สำหรับบัส 500 kbit/s หรือระบุขาของ transceiver ภายนอกด้วย `RB.canBegin(500000, txPin, rxPin)` ตัวอย่าง CANLoopback ใช้ normal mode ต้องมี CAN node อีกตัวรับและ ACK พร้อม termination ที่ปลายบัส

กำหนดค่าเริ่มต้น CAN ให้ตรงวงจร (TX=GPIO17, RX=GPIO16) และให้ SelfTest แสดงว่ากำหนดขาแล้วโดยไม่อ้างว่าฮาร์ดแวร์ผ่านการทดสอบ

## เซนเซอร์วัดระยะ

เพิ่มตัวช่วยเซนเซอร์ที่ใช้เพียง `#include <RB_Nexus.h>`:

| เซนเซอร์ | ประกาศอุปกรณ์ | อ่านค่า |
|---|---|---|
| HC-SR04 | `RBUltrasonicSensor front(26, 27);` | `front.cm()` |
| Sharp GP2Y0A21YK0F 10–80 cm | `RBIRDistanceSensor front(1);` (A1) | `front.cm()` / `front.volts()` |
| IR ดิจิทัล active LOW | `RBIRObstacleSensor front(4);` | `front.detected()` |

เรียก `RB.begin()` และ `front.begin()` ใน setup() ก่อนใช้ อ่านอัลตราโซนิกห่างกันอย่างน้อย
60 ms (ตัวอย่างใช้ 100 ms) และตรวจ `isfinite(cm)` ก่อนใช้ระยะ: ค่าอ่านไม่ได้เป็น `NaN`
พร้อม `statusText()` ไม่ใช่ 0 cm ส่วน IR ดิจิทัลให้เพียงพบ/ไม่พบ ไม่แปลงเป็นระยะ

HC-SR04 ต้องใช้ไฟ 5V, GND ร่วม และลด ECHO ก่อนเข้า ESP32 เช่น ECHO → 2.2kΩ → GPIO27
และ GPIO27 → 3.3kΩ → GND ห้ามต่อ ECHO 5V เข้าขา ESP32 ตรง ๆ
Sharp ใช้ไฟเลี้ยงที่เหมาะสมแยกจากขั้ว A1 ที่จ่าย 3.3V และสัญญาณ ADC ต้องไม่เกิน 3.3V
กราฟเริ่มต้นเป็นค่าประมาณ ต้องใช้ `setCalibration()` เทียบกับไม้บรรทัดสำหรับงานจริง
ระยะต่ำกว่า 10 cm ของ Sharp อาจให้ค่าเหมือนอยู่ไกล จึงไม่เหมาะใช้เดี่ยว ๆ เพื่อรับประกันการหยุดชน

ตัวอย่างใหม่: **DistanceUltrasonic**, **DistanceIRObstacle**, **DistanceIRAnalog**,
**DistanceIRCalibration**, **DistanceWarningLED**, **MotorBeginner**
ตัวอย่าง MotorBeginner รอคำสั่ง Serial ก่อนเคลื่อนที่ และหยุดเองใน 1 วินาที

## การทดสอบซอฟต์แวร์

```sh
python scripts/install_dependencies.py --config-file arduino-cli.yaml
python scripts/test_host.py
python -m unittest discover -s tests -p "test_*.py"
python scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY
python scripts/verify_package.py
python scripts/compile_examples.py --config-file arduino-cli.yaml
```

ยังต้องทดสอบบนบอร์ดจริง โดยเฉพาะ I2C ของ BNO085, calibration/แกน IMU และการหยุดมอเตอร์ รายงานเดิมระบุข้อจำกัด PCB ของ M1/M2/M3; การแก้ซอฟต์แวร์ไม่ได้ยืนยันว่าแก้ปัญหาวงจรแล้ว

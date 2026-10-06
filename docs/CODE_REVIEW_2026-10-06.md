# ผลตรวจโค้ด RB_Nexus

วันที่ 6 ตุลาคม 2026 — commit f62164a, version 0.2.0

ดาวน์โหลดจาก https://github.com/Sakda-Oil/RB_Nexus_LIBRARY.git

## ขอบเขตและผลทดสอบ

- ตรวจ driver หลัก, micro-ROS wrapper, ตัวอย่างที่เกี่ยวข้อง, README และขั้นตอน build/CI
- พบ 39 sketches; VERSION และ package index ระบุ 0.2.0 ตรงกัน
- `python -m compileall -q scripts` ผ่าน (ตรวจ syntax Python เท่านั้น)
- ทดลอง `python scripts/compile_examples.py --filter CompileCheck` แต่เริ่ม compiler ไม่ได้: ไม่มี Arduino CLI ใน PATH และ fallback ชี้ไป /opt/homebrew/bin/arduino-cli ซึ่งใช้ไม่ได้บนเครื่องนี้
- ยังไม่ได้ build/verify archive, compile ESP32 จริง หรือทดสอบฮาร์ดแวร์ ข้อความ PASS ในรายงานเดิมของ repository ไม่ใช่ผลทดสอบใหม่ครั้งนี้
- ไม่ได้แก้ implementation หรือเผยแพร่การเปลี่ยนแปลง

## ข้อค้นพบเรียงตามความสำคัญ

### 1. P1: micro-ROS ตัวจริงใช้กับตัวอย่างหลายไฟล์ไม่ได้

ตัวอย่าง 02, 03, 08, 09, 12, 13, 14, 15 เรียก `rclc_executor_init` ด้วย `&support.dummy` เช่น `libraries/RB_Nexus/examples/micro_ros/08_Motor_Subscriber/08_Motor_Subscriber.ino:30` แต่ rclc_support_t จริงมี `context` ไม่ใช่ `dummy` ต้องใช้ `&support.context` ตัวอย่าง 10 ที่บรรทัด 24–25 ใช้ `msg.frame_id` แทน `msg.header.frame_id` ด้วย

Fallback ใน RB_Nexus_MicroROS.h สร้าง dummy types และฟังก์ชัน no-op ที่คืน success จึงซ่อนปัญหานี้เมื่อไม่ได้ติดตั้ง micro_ros_arduino ส่วน CI ไม่ได้ติดตั้ง dependency ดังกล่าวโดยตรง จึงไม่เพียงพอที่จะยืนยันการใช้งาน ROS จริง ควรแยกการทดสอบ fallback และ integration พร้อม dependency จริง

อ้างอิง API ทางการ:
- https://docs.ros.org/en/humble/p/rclc/generated/structrclc__support__t.html
- https://raw.githubusercontent.com/ros2/common_interfaces/rolling/sensor_msgs/msg/Imu.msg

### 2. P1: คำสั่งหยุดมอเตอร์ถูก PID เขียนทับได้

`libraries/RB_Nexus/src/RB_Nexus.cpp:353` motorStop() ไม่ปิด PID หรือเคลียร์ target; motorStopAll() เรียกฟังก์ชันเดียวกัน เมื่อเปิด PID ด้วย motorSetRPM() แล้วสั่งหยุด รอบ updatePID() ถัดไปยังส่งกำลังให้มอเตอร์ได้ ควรแยกฟังก์ชันหยุดเอาต์พุตภายในกับ API หยุดที่ยกเลิกตัวควบคุม และทดสอบ stop ขณะ PID ทำงาน ปัจจุบัน motorPIDDisable() และ emergencyStop() ปิด PID อยู่แล้ว

### 3. P1: ตัวอย่างควบคุมมอเตอร์ไม่มี timeout ของคำสั่ง

ตัวอย่าง 08 และ 09 ไม่เปิด watchdog หรือตรวจเวลาคำสั่งล่าสุด ส่วน `15_AllSensors.ino:114` feedWatchdog() ทุก loop แม้ไม่มีข้อความจาก ROS หากการสื่อสารหยุดแต่ loop ยังเดิน เอาต์พุตมอเตอร์จะค้างตามคำสั่งล่าสุด ควรบันทึกเวลารับคำสั่งใน callback และหยุดเมื่อเกิน timeout พร้อมทดสอบถอดการเชื่อมต่อหลังสั่งหมุน

### 4. P1: IMU คืนศูนย์ตลอดแม้แจ้งว่าตรวจพบ

`libraries/RB_Nexus/src/RB_Nexus.cpp:587` imuBegin() ตรวจเพียง I2C ACK โดยไม่มีการตรวจ chip ID, ตั้งค่าเซนเซอร์ หรืออ่าน register ส่วน accel/gyro/angles ถูกตั้งเป็นศูนย์ใน constructor และไม่มีจุดอัปเดต ตัวอย่างจึงส่งค่าศูนย์ ไม่ใช่ข้อมูลเซนเซอร์ ควรทำ driver ตามชิปรุ่นจริง หรือรายงานว่ายังไม่รองรับแทนการแจ้งพร้อมใช้งาน

### 5. P2: README ใช้ API ที่ไม่มีอยู่

`README.md:61` เป็นต้นไปใช้ Nexus, encoderInit, motorDrive, analogReadMCP, analogReadVoltage, getEncoderCount และ getEncoderRPM ซึ่งไม่มีใน public API ต้องปรับเป็น RB และเมธอดที่ประกาศจริง นอกจากนี้ begin(115200) รับความถี่ I2C ไม่ใช่ serial baud; ต้องเรียก Serial.begin(115200) แยกต่างหาก และเรียก RB.update() เพื่ออัปเดต RPM

### 6. P2: global micro-ROS object มีแต่ declaration

`libraries/RB_Nexus/src/RB_Nexus_MicroROS.h:184` ประกาศ extern RBNexusMicroROS RBMicroROS แต่ไม่มี definition ในไฟล์ implementation การเรียกใช้ object นี้จะเกิด undefined reference ขณะ link ควรเพิ่ม definition หนึ่งจุด อีกทั้ง beginSerial()/beginWiFi() ของ wrapper ยังไม่เรียก transport setup และ beginWiFi() ไม่ใช้ agentIP/agentPort

### 7. P2: servo pulse ไม่ตามความถี่ PCA9685

`libraries/RB_Nexus/src/RB_Nexus.cpp:459` แปลง microseconds โดยสมมติ period 20000 us เสมอ แต่เปิด API setPWMFreq() ให้เปลี่ยนความถี่ได้ เช่นตั้ง 100 Hz แล้วสั่ง 1500 us จะได้พัลส์ประมาณ 750 us ควรเก็บความถี่จริงเพื่อคำนวณ หรือจำกัดการใช้ servo ที่ 50 Hz อย่างชัดเจน

### 8. P2: RB และ RB_Nexus เป็นคนละ instance

`libraries/RB_Nexus/src/RB_Nexus.cpp:720` สร้าง object สองตัวที่มีสถานะ PCA, PID และ emergency stop แยกกัน แม้ควบคุมฮาร์ดแวร์ชุดเดียวกัน เช่น begin() ผ่าน RB แล้วสั่งผ่าน RB_Nexus จะไม่ส่ง PWM เพราะสถานะ _pcaConnected ของตัวหลังยัง false ควรใช้อินสแตนซ์เดียวและให้ชื่อรองเป็น reference alias

### 9. P2: ค่าความเร็วมอเตอร์เกิน 255 ทำให้สเกลเปลี่ยนทันที

`libraries/RB_Nexus/src/RB_Nexus.cpp:328` motorSet(1,255) ให้ duty 4095 แต่ motorSet(1,256) ให้ duty 256 เนื่องจากสลับไปตีความเป็น raw PWM ทั้งที่ header ระบุช่วง -255..255 และตัวอย่าง ROS รับ Int16 โดยไม่ clamp ควร clamp ให้ตรงสัญญา API หรือแยกฟังก์ชัน raw duty ต่างหาก

## ข้อสรุป

ยังยืนยันว่าโปรแกรมถูกต้องหรือพร้อมใช้งานจริงไม่ได้ มีข้อผิดพลาดชัดเจนทั้งการเชื่อมต่อ ROS และพฤติกรรมขณะทำงาน ควรแก้ P1 ก่อน จากนั้น compile ทั้ง 39 ตัวอย่างด้วย Arduino-ESP32 และ micro_ros_arduino จริง แล้วทดสอบมอเตอร์/encoder/IMU บนบอร์ดตามรุ่นฮาร์ดแวร์

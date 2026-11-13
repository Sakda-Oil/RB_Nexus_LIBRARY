#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// =============================================================================
// All sensors and actuators in one node
// =============================================================================
// Publishers:
//   /rb_nexus/analog      A1..A8 raw (Int32MultiArray, 20 Hz)
//   /rb_nexus/encoder/1   ticks (Int32, 50 Hz)
//   /rb_nexus/imu         raw IMU (sensor_msgs/Imu, 50 Hz)
//   /rb_nexus/battery     A8 x 11 divider (BatteryState, 1 Hz)
//   /rb_nexus/status      1 = OK, 99 = emergency stop (Int32, 1 Hz)
// Subscribers:
//   /rb_nexus/motor/1/cmd    motor 1 power -100..100 % (Int16, send >= 10 Hz)
//   /rb_nexus/servo/1/cmd    servo 1 angle 0..180 (Int16)
//   /rb_nexus/pwm/cmd        D4 PWM 0..100 % (Int16)
//   /rb_nexus/emergency_stop true = stop and latch, false = release (Bool)
// =============================================================================

RBMotorCommandGuard motorCommands(500);

rcl_publisher_t analogPub, encoderPub, imuPub, batteryPub, statusPub;
rcl_subscription_t motorSub, servoSub, pwmSub, estopSub;

std_msgs__msg__Int32MultiArray analog;
int32_t analogChannels[8];
std_msgs__msg__Int32 ticks, status;
sensor_msgs__msg__Imu imu;
sensor_msgs__msg__BatteryState battery;
std_msgs__msg__Int16 motorCommand, servoCommand, pwmCommand;
std_msgs__msg__Bool estopCommand;
char imuFrame[] = "imu_link";

void onMotor(const void* message) {
  RB.motorPower(1, ((const std_msgs__msg__Int16*)message)->data);
  motorCommands.feed();
}

void onServo(const void* message) {
  RB.servoAngle(1, constrain(((const std_msgs__msg__Int16*)message)->data, 0, 180));
}

void onPWM(const void* message) {
  RB.pinPWM(RB_PIN_D4, constrain(((const std_msgs__msg__Int16*)message)->data, 0, 100));
}

void onEmergency(const void* message) {
  if (((const std_msgs__msg__Bool*)message)->data) RB.emergencyStop();
  else RB.emergencyClear();
}

bool createROS() {
  return RBROS.publisher(analogPub, RB_MSG(std_msgs, Int32MultiArray), RB_TOPIC_ANALOG_MULTI) &&
         RBROS.publisher(encoderPub, RB_MSG(std_msgs, Int32), RB_TOPIC_ENCODER_CH1) &&
         RBROS.publisher(imuPub, RB_MSG(sensor_msgs, Imu), RB_TOPIC_IMU_RAW) &&
         RBROS.publisher(batteryPub, RB_MSG(sensor_msgs, BatteryState), RB_TOPIC_BATTERY) &&
         RBROS.publisher(statusPub, RB_MSG(std_msgs, Int32), RB_TOPIC_ROBOT_STATUS) &&
         RBROS.subscriber(motorSub, RB_MSG(std_msgs, Int16), RB_TOPIC_MOTOR_1_CMD, &motorCommand, onMotor) &&
         RBROS.subscriber(servoSub, RB_MSG(std_msgs, Int16), RB_TOPIC_SERVO_1_CMD, &servoCommand, onServo) &&
         RBROS.subscriber(pwmSub, RB_MSG(std_msgs, Int16), RB_TOPIC_PWM_CMD, &pwmCommand, onPWM) &&
         RBROS.subscriber(estopSub, RB_MSG(std_msgs, Bool), RB_TOPIC_EMERGENCY_STOP, &estopCommand, onEmergency);
}

void setup() {
  RB.begin();
  RB.encoderBegin();
  RB.imuBegin();
  RB.stop();

  analog.data.data = analogChannels;
  analog.data.size = 8;
  analog.data.capacity = 8;
  imu.header.frame_id.data = imuFrame;
  imu.header.frame_id.size = sizeof(imuFrame) - 1;
  imu.header.frame_id.capacity = sizeof(imuFrame);
  battery.present = true;
  battery.current = battery.charge = battery.capacity = battery.design_capacity = NAN;
  battery.temperature = battery.percentage = NAN;

  RBROS.begin("rb_nexus_robot_node", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
  motorCommands.update();
  if (!RBROS.connected()) return;

  const uint32_t now = millis();

  static uint32_t lastFast = 0;   // 50 Hz: encoder and IMU
  if (now - lastFast >= 20) {
    lastFast = now;
    ticks.data = RB.encoderTicks(1);
    RBROS.publish(encoderPub, &ticks);
    if (RBROS.fillImu(imu)) RBROS.publish(imuPub, &imu);
  }

  static uint32_t lastMedium = 0; // 20 Hz: analog
  if (now - lastMedium >= 50) {
    lastMedium = now;
    for (uint8_t ch = 1; ch <= 8; ++ch) analogChannels[ch - 1] = RB.analogRaw(ch);
    RBROS.publish(analogPub, &analog);
  }

  static uint32_t lastSlow = 0;   // 1 Hz: battery and status
  if (now - lastSlow >= 1000) {
    lastSlow = now;
    battery.voltage = RB.analogVolt(8) * 11.0f;
    RBROS.stamp(battery.header.stamp);
    RBROS.publish(batteryPub, &battery);
    status.data = RB.emergencyActive() ? 99 : 1;
    RBROS.publish(statusPub, &status);
    RB.ledToggle();
  }
}

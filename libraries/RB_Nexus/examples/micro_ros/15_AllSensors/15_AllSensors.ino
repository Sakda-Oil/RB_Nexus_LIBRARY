#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

RBMotorCommandGuard motorCommands(500);

// =============================================================================
// micro-ROS AllSensors & AllActuators Example for RB_Nexus
// =============================================================================
// Publishers:
//   - /rb_nexus/analog (MCP3208 CH1 - CH8, 20 Hz)
//   - /rb_nexus/encoder/1 (50 Hz)
//   - /rb_nexus/imu (IMU Acceleration & Gyro, 50 Hz)
//   - /rb_nexus/battery (Battery voltage estimate, 1 Hz)
//   - /rb_nexus/status (System diagnostic status, 10 Hz)
//
// Subscribers:
//   - /rb_nexus/motor/1/cmd (Int16)
//   - /rb_nexus/servo/1/cmd (Int16)
//   - /rb_nexus/pwm/cmd (Int16)
//   - /rb_nexus/emergency_stop (Bool)
// =============================================================================

// Publishers
rcl_publisher_t pub_analog;
rcl_publisher_t pub_encoder;
rcl_publisher_t pub_imu;
rcl_publisher_t pub_battery;
rcl_publisher_t pub_status;

// Subscribers
rcl_subscription_t sub_motor;
rcl_subscription_t sub_servo;
rcl_subscription_t sub_pwm;
rcl_subscription_t sub_estop;

// Messages
std_msgs__msg__Int32MultiArray msg_analog;
int32_t analog_buf[8];
std_msgs__msg__Int32           msg_encoder;
sensor_msgs__msg__Imu          msg_imu;
sensor_msgs__msg__BatteryState msg_battery;
std_msgs__msg__Int32           msg_status;

std_msgs__msg__Int16           cmd_motor;
std_msgs__msg__Int16           cmd_servo;
std_msgs__msg__Int16           cmd_pwm;
std_msgs__msg__Bool            cmd_estop;

rclc_executor_t executor;
rclc_support_t  support;
rcl_allocator_t allocator;
rcl_node_t      node;

void cb_motor(const void* msgin) {
  const std_msgs__msg__Int16* msg = (const std_msgs__msg__Int16*)msgin;
  RB.motorSet(1, msg->data);
  motorCommands.feed();
}

void cb_servo(const void* msgin) {
  const std_msgs__msg__Int16* msg = (const std_msgs__msg__Int16*)msgin;
  RB.servoWrite(8, constrain(msg->data, 0, 180));
}

void cb_pwm(const void* msgin) {
  const std_msgs__msg__Int16* msg = (const std_msgs__msg__Int16*)msgin;
  RB.pwmWrite(RB_PIN_D4, constrain(msg->data, 0, 255));
}

void cb_estop(const void* msgin) {
  const std_msgs__msg__Bool* msg = (const std_msgs__msg__Bool*)msgin;
  if (msg->data) {
    RB.emergencyStop();
  } else {
    RB.clearEmergencyStop();
  }
}

void setup() {
  Serial.begin(115200);
  RB.begin();
  RB.encoderBegin();
  RB.imuBegin();
  RB.motorStopAll();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_robot_node", "", &support));

  // Initialize Publishers
  rbROSCheck(rclc_publisher_init_default(&pub_analog, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray), RB_TOPIC_ANALOG_MULTI));
  rbROSCheck(rclc_publisher_init_default(&pub_encoder, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), RB_TOPIC_ENCODER_CH1));
  rbROSCheck(rclc_publisher_init_default(&pub_imu, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu), RB_TOPIC_IMU_RAW));
  rbROSCheck(rclc_publisher_init_default(&pub_battery, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState), RB_TOPIC_BATTERY));
  rbROSCheck(rclc_publisher_init_default(&pub_status, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), RB_TOPIC_ROBOT_STATUS));

  msg_analog.data.data = analog_buf;
  msg_analog.data.size = 8;
  msg_analog.data.capacity = 8;

  static char imuFrame[] = "imu_link";
  msg_imu.header.frame_id.data = imuFrame;
  msg_imu.header.frame_id.size = sizeof(imuFrame) - 1;
  msg_imu.header.frame_id.capacity = sizeof(imuFrame);
  rmw_uros_sync_session(1000);

  // Initialize Subscribers
  rbROSCheck(rclc_subscription_init_default(&sub_motor, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16), RB_TOPIC_MOTOR_1_CMD));
  rbROSCheck(rclc_subscription_init_default(&sub_servo, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16), RB_TOPIC_SERVO_1_CMD));
  rbROSCheck(rclc_subscription_init_default(&sub_pwm, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16), RB_TOPIC_PWM_CMD));
  rbROSCheck(rclc_subscription_init_default(&sub_estop, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), "/rb_nexus/emergency_stop"));

  rbROSCheck(rclc_executor_init(&executor, &support.context, 4, &allocator));
  rbROSCheck(rclc_executor_add_subscription(&executor, &sub_motor, &cmd_motor, &cb_motor, ON_NEW_DATA));
  rbROSCheck(rclc_executor_add_subscription(&executor, &sub_servo, &cmd_servo, &cb_servo, ON_NEW_DATA));
  rbROSCheck(rclc_executor_add_subscription(&executor, &sub_pwm, &cmd_pwm, &cb_pwm, ON_NEW_DATA));
  rbROSCheck(rclc_executor_add_subscription(&executor, &sub_estop, &cmd_estop, &cb_estop, ON_NEW_DATA));
}

void loop() {
  motorCommands.update();
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);

  unsigned long now = millis();

  // 1. Encoder & IMU (50 Hz / 20ms)
  static unsigned long lastFast = 0;
  if (now - lastFast >= 20) {
    lastFast = now;
    msg_encoder.data = RB.encoderRead(1);
    rcl_publish(&pub_encoder, &msg_encoder, NULL);

    if (rbFillImuMessage(msg_imu)) rcl_publish(&pub_imu, &msg_imu, NULL);
  }

  // 2. Analog MCP3208 (20 Hz / 50ms)
  static unsigned long lastMedium = 0;
  if (now - lastMedium >= 50) {
    lastMedium = now;
    for (uint8_t i = 0; i < 8; ++i) {
      analog_buf[i] = RB.analogRead(i + 1);
    }
    rcl_publish(&pub_analog, &msg_analog, NULL);
  }

  // 3. Status & Battery (1 Hz / 1000ms)
  static unsigned long lastSlow = 0;
  if (now - lastSlow >= 1000) {
    lastSlow = now;
    msg_battery.voltage = RB.analogVoltage(8) * 11.0f; // Scale factor
    rcl_publish(&pub_battery, &msg_battery, NULL);

    msg_status.data = RB.isEmergencyStopped() ? 99 : 1;
    rcl_publish(&pub_status, &msg_status, NULL);
    RB.toggleLED();
  }
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

RBMotorCommandGuard motorCommands(500);

rcl_subscription_t sub_motor;
rcl_publisher_t    pub_encoder;
std_msgs__msg__Int16 msg_motor;
std_msgs__msg__Int32 msg_encoder;

rclc_executor_t executor;
rclc_support_t  support;
rcl_allocator_t allocator;
rcl_node_t      node;

void motor_callback(const void* msgin) {
  const std_msgs__msg__Int16* cmd = (const std_msgs__msg__Int16*)msgin;
  RB.motorSet(1, cmd->data);
  motorCommands.feed();
}

void setup() {
  Serial.begin(115200);
  RB.begin();
  RB.encoderBegin();
  RB.motorStopAll();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_motor_encoder", "", &support));

  rbROSCheck(rclc_subscription_init_default(
    &sub_motor, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16),
    RB_TOPIC_MOTOR_1_CMD));

  rbROSCheck(rclc_publisher_init_default(
    &pub_encoder, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    RB_TOPIC_ENCODER_CH1));

  rbROSCheck(rclc_executor_init(&executor, &support.context, 1, &allocator));
  rbROSCheck(rclc_executor_add_subscription(&executor, &sub_motor, &msg_motor, &motor_callback, ON_NEW_DATA));
}

void loop() {
  motorCommands.update();
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);

  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 20) { // 50 Hz feedback
    lastPub = millis();
    msg_encoder.data = RB.encoderRead(1);
    rcl_publish(&pub_encoder, &msg_encoder, NULL);
  }
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_subscription_t subscriber;
std_msgs__msg__Int16 msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void pwm_callback(const void* msgin) {
  const std_msgs__msg__Int16* msg = (const std_msgs__msg__Int16*)msgin;
  uint8_t duty = (uint8_t)constrain(msg->data, 0, 255);
  RB.pwmWrite(RB_PIN_D4, duty);
}

void setup() {
  RB.begin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rb_nexus_pwm_sub", "", &support);
  rclc_subscription_init_default(
    &subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16),
    RB_TOPIC_PWM_CMD);

  rclc_executor_init(&executor, &support.dummy, 1, &allocator);
  rclc_executor_add_subscription(&executor, &subscriber, &msg, &pwm_callback, ON_NEW_DATA);
}

void loop() {
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);
}

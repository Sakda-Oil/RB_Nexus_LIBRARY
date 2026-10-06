#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_subscription_t subscriber;
std_msgs__msg__Bool msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void led_callback(const void* msgin) {
  const std_msgs__msg__Bool* msg = (const std_msgs__msg__Bool*)msgin;
  RB.setLED(msg->data);
}

void setup() {
  Serial.begin(115200);
  RB.begin();
  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_led_sub", "", &support));
  rbROSCheck(rclc_subscription_init_default(
    &subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
    RB_TOPIC_LED_CMD));

  rbROSCheck(rclc_executor_init(&executor, &support.context, 1, &allocator));
  rbROSCheck(rclc_executor_add_subscription(&executor, &subscriber, &msg, &led_callback, ON_NEW_DATA));
}

void loop() {
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);
}

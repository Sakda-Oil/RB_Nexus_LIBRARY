#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_subscription_t subscriber;
std_msgs__msg__Int32 msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void subscription_callback(const void* msgin) {
  const std_msgs__msg__Int32* msg = (const std_msgs__msg__Int32*)msgin;
  if (msg->data > 0) {
    RB.setLED(true);
  } else {
    RB.setLED(false);
  }
}

void setup() {
  Serial.begin(115200);
  RB.begin();
  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_subscriber", "", &support));
  rbROSCheck(rclc_subscription_init_default(
    &subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "/rb_nexus/cmd_int"));

  rbROSCheck(rclc_executor_init(&executor, &support.context, 1, &allocator));
  rbROSCheck(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));
}

void loop() {
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);
}

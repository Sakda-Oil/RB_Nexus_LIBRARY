#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_subscription_t subscriber;
std_msgs__msg__Int16 msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void motor_callback(const void* msgin) {
  const std_msgs__msg__Int16* msg = (const std_msgs__msg__Int16*)msgin;
  // Speed is -255 to +255
  RB.motorSet(1, msg->data);
}

void setup() {
  RB.begin();
  RB.motorStopAll();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rb_nexus_motor_sub", "", &support);
  rclc_subscription_init_default(
    &subscriber, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int16),
    RB_TOPIC_MOTOR_1_CMD);

  rclc_executor_init(&executor, &support.dummy, 1, &allocator);
  rclc_executor_add_subscription(&executor, &subscriber, &msg, &motor_callback, ON_NEW_DATA);
}

void loop() {
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// micro-ROS Serial Transport via USB-UART (CH340) at 115200 baud
rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  RB.begin();

  // Set default Serial transport
  set_microros_transports();

  allocator = rcutils_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rb_nexus_serial_node", "", &support);
  rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "/rb_nexus/serial_test");

  msg.data = 0;
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 1000) {
    lastPub = millis();
    msg.data++;
    rcl_publish(&publisher, &msg, NULL);
    RB.toggleLED();
  }
}

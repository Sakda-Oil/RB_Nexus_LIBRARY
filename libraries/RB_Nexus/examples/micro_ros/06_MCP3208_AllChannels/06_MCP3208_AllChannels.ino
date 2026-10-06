#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t publisher;
std_msgs__msg__Int32MultiArray msg;
int32_t channel_data[8];
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  Serial.begin(115200);
  RB.begin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_mcp3208_pub", "", &support));
  rbROSCheck(rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
    RB_TOPIC_ANALOG_MULTI));

  msg.data.data = channel_data;
  msg.data.size = 8;
  msg.data.capacity = 8;
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 100) { // 10 Hz
    lastPub = millis();
    for (uint8_t ch = 1; ch <= 8; ++ch) {
      channel_data[ch - 1] = RB.analogRead(ch);
    }
    rcl_publish(&publisher, &msg, NULL);
    RB.toggleLED();
  }
}

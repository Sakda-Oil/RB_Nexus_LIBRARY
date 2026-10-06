#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t publisher;
std_msgs__msg__Bool msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  Serial.begin(115200);
  RB.begin();
  RB.pinMode(RB_PIN_D4, INPUT_PULLUP);

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_button_pub", "", &support));
  rbROSCheck(rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
    RB_TOPIC_BUTTON_STATE));
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 50) { // 20 Hz
    lastPub = millis();
    // Invert because INPUT_PULLUP: pressed = LOW -> True
    msg.data = (RB.digitalRead(RB_PIN_D4) == LOW);
    rcl_publish(&publisher, &msg, NULL);
  }
}

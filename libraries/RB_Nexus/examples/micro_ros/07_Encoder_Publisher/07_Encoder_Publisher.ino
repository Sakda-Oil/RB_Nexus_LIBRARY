#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t pub_enc1;
std_msgs__msg__Int32 msg_enc1;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  RB.begin();
  RB.encoderBegin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rb_nexus_encoder_pub", "", &support);
  rclc_publisher_init_default(
    &pub_enc1, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    RB_TOPIC_ENCODER_CH1);
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 20) { // 50 Hz
    lastPub = millis();
    msg_enc1.data = RB.encoderRead(1);
    rcl_publish(&pub_enc1, &msg_enc1, NULL);
  }
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t    pub_input;
rcl_subscription_t sub_output;
std_msgs__msg__Int32 msg_in;
std_msgs__msg__Int32 msg_out;

rclc_executor_t executor;
rclc_support_t  support;
rcl_allocator_t allocator;
rcl_node_t      node;

void output_callback(const void* msgin) {
  const std_msgs__msg__Int32* msg = (const std_msgs__msg__Int32*)msgin;
  // Bit 0 -> Pin 4, Bit 1 -> Pin 12, Bit 2 -> Pin 14, Bit 3 -> Pin 26, Bit 4 -> Pin 27
  RB.digitalWrite(RB_PIN_D4,  (msg->data & 0x01) ? HIGH : LOW);
  RB.digitalWrite(RB_PIN_D12, (msg->data & 0x02) ? HIGH : LOW);
  RB.digitalWrite(RB_PIN_D14, (msg->data & 0x04) ? HIGH : LOW);
  RB.digitalWrite(RB_PIN_D26, (msg->data & 0x08) ? HIGH : LOW);
  RB.digitalWrite(RB_PIN_D27, (msg->data & 0x10) ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  RB.begin();
  RB.pinMode(RB_PIN_D4,  INPUT_PULLUP);
  RB.pinMode(RB_PIN_D12, OUTPUT);
  RB.pinMode(RB_PIN_D14, OUTPUT);
  RB.pinMode(RB_PIN_D26, OUTPUT);
  RB.pinMode(RB_PIN_D27, OUTPUT);

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_digital_io", "", &support));

  rbROSCheck(rclc_publisher_init_default(
    &pub_input, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    RB_TOPIC_DIGITAL_IN));

  rbROSCheck(rclc_subscription_init_default(
    &sub_output, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    RB_TOPIC_DIGITAL_OUT));

  rbROSCheck(rclc_executor_init(&executor, &support.context, 1, &allocator));
  rbROSCheck(rclc_executor_add_subscription(&executor, &sub_output, &msg_out, &output_callback, ON_NEW_DATA));
}

void loop() {
  RB.update();
  rclc_executor_spin_some(&executor, 1000000);

  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 100) {
    lastPub = millis();
    msg_in.data = RB.digitalRead(RB_PIN_D4);
    rcl_publish(&pub_input, &msg_in, NULL);
  }
}

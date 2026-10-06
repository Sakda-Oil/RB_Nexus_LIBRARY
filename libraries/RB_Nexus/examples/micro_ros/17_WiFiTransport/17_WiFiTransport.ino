#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// micro-ROS Wi-Fi UDP Transport Configuration
char ssid[] = "Your_WiFi_SSID";
char pass[] = "Your_WiFi_Password";
IPAddress agent_ip(192, 168, 1, 100); // IP Address of ROS 2 micro-ROS Agent
size_t agent_port = 8888;

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  Serial.begin(115200);
  RB.begin();

  // Set Wi-Fi transport for micro-ROS
  if (!RBMicroROS.beginWiFi(ssid, pass, agent_ip, agent_port)) {
    RB.emergencyStop();
    while (true) delay(100);
  }

  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_wifi_node", "", &support));
  rbROSCheck(rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "/rb_nexus/wifi_test"));

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

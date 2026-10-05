#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t publisher;
std_msgs__msg__Int32MultiArray msg;
int32_t status_array[5];
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  RB.begin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rb_nexus_status_pub", "", &support);
  rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
    RB_TOPIC_ROBOT_STATUS);

  msg.data.data = status_array;
  msg.data.size = 5;
  msg.data.capacity = 5;
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 1000) { // 1 Hz
    lastPub = millis();

    // 0: Uptime (seconds)
    status_array[0] = millis() / 1000;
    // 1: Free Heap (bytes)
    status_array[1] = ESP.getFreeHeap();
    // 2: WiFi RSSI (dBm)
    status_array[2] = RB.wifiRSSI();
    // 3: Emergency Stop State (0=Normal, 1=E-STOP)
    status_array[3] = RB.isEmergencyStopped() ? 1 : 0;
    // 4: Battery Raw ADC
    status_array[4] = RB.analogRead(8);

    rcl_publish(&publisher, &msg, NULL);
    RB.toggleLED();
  }
}

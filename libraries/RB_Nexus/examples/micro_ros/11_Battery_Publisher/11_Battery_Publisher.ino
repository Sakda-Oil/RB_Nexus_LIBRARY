#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t publisher;
sensor_msgs__msg__BatteryState msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  Serial.begin(115200);
  RB.begin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_battery_pub", "", &support));
  rbROSCheck(rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState),
    RB_TOPIC_BATTERY));

  msg.power_supply_status = 0; // Unknown / Discharging
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 1000) { // 1 Hz
    lastPub = millis();
    // Example: Read supply voltage divider from MCP3208 Channel 8 (A8)
    // Assume 10:1 resistor divider:
    float rawV = RB.analogVoltage(8);
    msg.voltage = rawV * 11.0f; // Scale factor for battery divider
    msg.percentage = constrain((msg.voltage - 10.0f) / (12.6f - 10.0f), 0.0f, 1.0f);
    rcl_publish(&publisher, &msg, NULL);
  }
}

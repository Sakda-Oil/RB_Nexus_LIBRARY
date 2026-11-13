#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Publishes [uptime s, free heap bytes, Wi-Fi RSSI dBm, emergency 0/1, A8 raw] at 1 Hz.
rcl_publisher_t statusPub;
std_msgs__msg__Int32MultiArray status;
int32_t values[5];

bool createROS() {
  return RBROS.publisher(statusPub, RB_MSG(std_msgs, Int32MultiArray), RB_TOPIC_ROBOT_STATUS);
}

void setup() {
  RB.begin();
  status.data.data = values;
  status.data.size = 5;
  status.data.capacity = 5;
  RBROS.begin("rb_nexus_status_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 1000) {
    last = millis();
    values[0] = millis() / 1000;
    values[1] = ESP.getFreeHeap();
    values[2] = RB.wifiConnected() ? RB.wifiRSSI() : 0;
    values[3] = RB.emergencyActive() ? 1 : 0;
    values[4] = RB.analogRaw(8);
    RBROS.publish(statusPub, &status);
    RB.ledToggle();
  }
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Publishes analog A1..A8 (raw 0..4095) as one Int32MultiArray at 10 Hz.
rcl_publisher_t analogPub;
std_msgs__msg__Int32MultiArray analog;
int32_t channels[8];

bool createROS() {
  return RBROS.publisher(analogPub, RB_MSG(std_msgs, Int32MultiArray), RB_TOPIC_ANALOG_MULTI);
}

void setup() {
  RB.begin();
  analog.data.data = channels;
  analog.data.size = 8;
  analog.data.capacity = 8;
  RBROS.begin("rb_nexus_mcp3208_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 100) {
    last = millis();
    for (uint8_t ch = 1; ch <= 8; ++ch) channels[ch - 1] = RB.analogRaw(ch);
    RBROS.publish(analogPub, &analog);
    RB.ledToggle();
  }
}

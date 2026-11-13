#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Publishes a counter on /rb_nexus/counter once per second.
rcl_publisher_t counterPub;
std_msgs__msg__Int32 counter;

bool createROS() {
  return RBROS.publisher(counterPub, RB_MSG(std_msgs, Int32), "/rb_nexus/counter");
}

void setup() {
  RB.begin();
  RBROS.begin("rb_nexus_publisher", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 1000) {
    last = millis();
    counter.data++;
    RBROS.publish(counterPub, &counter);
    RB.ledToggle();
  }
}

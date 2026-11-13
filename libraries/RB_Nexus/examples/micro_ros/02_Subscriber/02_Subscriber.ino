#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// LED on when /rb_nexus/cmd_int > 0, off otherwise.
//   ros2 topic pub --once /rb_nexus/cmd_int std_msgs/msg/Int32 "{data: 1}"
rcl_subscription_t commandSub;
std_msgs__msg__Int32 command;

void onCommand(const void* message) {
  const auto* msg = (const std_msgs__msg__Int32*)message;
  RB.led(msg->data > 0);
}

bool createROS() {
  return RBROS.subscriber(commandSub, RB_MSG(std_msgs, Int32), "/rb_nexus/cmd_int", &command, onCommand);
}

void setup() {
  RB.begin();
  RBROS.begin("rb_nexus_subscriber", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Status LED follows /rb_nexus/led/cmd (std_msgs/Bool).
//   ros2 topic pub --once /rb_nexus/led/cmd std_msgs/msg/Bool "{data: true}"
rcl_subscription_t ledSub;
std_msgs__msg__Bool ledCommand;

void onLed(const void* message) {
  RB.led(((const std_msgs__msg__Bool*)message)->data);
}

bool createROS() {
  return RBROS.subscriber(ledSub, RB_MSG(std_msgs, Bool), RB_TOPIC_LED_CMD, &ledCommand, onLed);
}

void setup() {
  RB.begin();
  RBROS.begin("rb_nexus_led_sub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
}

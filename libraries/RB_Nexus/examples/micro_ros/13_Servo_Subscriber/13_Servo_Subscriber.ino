#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Servo 1 (PCA9685 CH8) angle 0..180 on /rb_nexus/servo/1/cmd (std_msgs/Int16).
//   ros2 topic pub --once /rb_nexus/servo/1/cmd std_msgs/msg/Int16 "{data: 45}"
rcl_subscription_t servoSub;
std_msgs__msg__Int16 servoCommand;

void onServo(const void* message) {
  RB.servoAngle(1, constrain(((const std_msgs__msg__Int16*)message)->data, 0, 180));
}

bool createROS() {
  return RBROS.subscriber(servoSub, RB_MSG(std_msgs, Int16), RB_TOPIC_SERVO_1_CMD, &servoCommand, onServo);
}

void setup() {
  RB.begin();
  RB.servoAngle(1, 90);
  RBROS.begin("rb_nexus_servo_sub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
}

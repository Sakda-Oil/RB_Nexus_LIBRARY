#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// PWM on D4 in percent (0..100) from /rb_nexus/pwm/cmd (std_msgs/Int16).
//   ros2 topic pub --once /rb_nexus/pwm/cmd std_msgs/msg/Int16 "{data: 50}"
rcl_subscription_t pwmSub;
std_msgs__msg__Int16 pwmCommand;

void onPWM(const void* message) {
  RB.pinPWM(RB_PIN_D4, constrain(((const std_msgs__msg__Int16*)message)->data, 0, 100));
}

bool createROS() {
  return RBROS.subscriber(pwmSub, RB_MSG(std_msgs, Int16), RB_TOPIC_PWM_CMD, &pwmCommand, onPWM);
}

void setup() {
  RB.begin();
  RBROS.begin("rb_nexus_pwm_sub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
}

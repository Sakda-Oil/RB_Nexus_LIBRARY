#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Motor 1 power in percent (-100..100) on /rb_nexus/motor/1/cmd (std_msgs/Int16).
// Send commands at 10 Hz or faster: the motor stops 500 ms after the last one,
// and stops at once if the agent connection is lost.
//   ros2 topic pub -r 10 /rb_nexus/motor/1/cmd std_msgs/msg/Int16 "{data: 30}"
// Encoder 1 ticks are published back at 50 Hz.
RBMotorCommandGuard motorCommands(500);
rcl_subscription_t motorSub;
rcl_publisher_t encoderPub;
std_msgs__msg__Int16 motorCommand;
std_msgs__msg__Int32 ticks;

void onMotor(const void* message) {
  RB.motorPower(1, ((const std_msgs__msg__Int16*)message)->data);
  motorCommands.feed();
}

bool createROS() {
  return RBROS.subscriber(motorSub, RB_MSG(std_msgs, Int16), RB_TOPIC_MOTOR_1_CMD, &motorCommand, onMotor) &&
         RBROS.publisher(encoderPub, RB_MSG(std_msgs, Int32), RB_TOPIC_ENCODER_CH1);
}

void setup() {
  RB.begin();
  RB.encoderBegin();
  RB.stop();
  RBROS.begin("rb_nexus_motor_encoder", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
  motorCommands.update();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 20) {
    last = millis();
    ticks.data = RB.encoderTicks(1);
    RBROS.publish(encoderPub, &ticks);
  }
}

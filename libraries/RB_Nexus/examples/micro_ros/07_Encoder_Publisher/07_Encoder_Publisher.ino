#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Publishes encoder 1 ticks at 50 Hz.
rcl_publisher_t encoderPub;
std_msgs__msg__Int32 ticks;

bool createROS() {
  return RBROS.publisher(encoderPub, RB_MSG(std_msgs, Int32), RB_TOPIC_ENCODER_CH1);
}

void setup() {
  RB.begin();
  RB.encoderBegin();
  RBROS.begin("rb_nexus_encoder_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 20) {
    last = millis();
    ticks.data = RB.encoderTicks(1);
    RBROS.publish(encoderPub, &ticks);
  }
}

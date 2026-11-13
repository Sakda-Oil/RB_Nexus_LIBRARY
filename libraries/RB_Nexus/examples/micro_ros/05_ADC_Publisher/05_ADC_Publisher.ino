#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Publishes analog A1 (raw 0..4095) at 10 Hz.
rcl_publisher_t analogPub;
std_msgs__msg__Int32 analog;

bool createROS() {
  return RBROS.publisher(analogPub, RB_MSG(std_msgs, Int32), RB_TOPIC_ANALOG_CH1);
}

void setup() {
  RB.begin();
  RBROS.begin("rb_nexus_adc_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 100) {
    last = millis();
    analog.data = RB.analogRaw(1);
    RBROS.publish(analogPub, &analog);
  }
}

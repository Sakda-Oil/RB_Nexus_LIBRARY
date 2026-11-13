#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// micro-ROS over USB Serial (CH340) with an explicit baud rate.
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The status LED is ON while connected and OFF while waiting for the agent.
// Do not print to Serial: it carries ROS data.
rcl_publisher_t counterPub;
std_msgs__msg__Int32 counter;

bool createROS() {
  return RBROS.publisher(counterPub, RB_MSG(std_msgs, Int32), "/rb_nexus/serial_test");
}

void setup() {
  RB.begin();
  RBROS.useSerial(115200);
  RBROS.begin("rb_nexus_serial_node", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();
  RB.led(RBROS.connected());

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 1000) {
    last = millis();
    counter.data++;
    RBROS.publish(counterPub, &counter);
  }
}

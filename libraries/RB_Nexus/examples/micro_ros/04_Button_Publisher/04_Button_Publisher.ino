#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Button between D4 and GND. Publishes true while pressed, 20 Hz.
const uint8_t BUTTON_PIN = RB_PIN_D4;
rcl_publisher_t buttonPub;
std_msgs__msg__Bool button;

bool createROS() {
  return RBROS.publisher(buttonPub, RB_MSG(std_msgs, Bool), RB_TOPIC_BUTTON_STATE);
}

void setup() {
  RB.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  RBROS.begin("rb_nexus_button_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 50) {
    last = millis();
    button.data = !RB.pinRead(BUTTON_PIN); // Pressed = LOW
    RBROS.publish(buttonPub, &button);
  }
}

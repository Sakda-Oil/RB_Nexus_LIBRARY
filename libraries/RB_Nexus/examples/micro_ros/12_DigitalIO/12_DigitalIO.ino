#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Input: D4 (button to GND) published on /rb_nexus/digital/input (1 = HIGH).
// Outputs: /rb_nexus/digital/output bits -> bit0 D12, bit1 D14, bit2 D26, bit3 D27.
//   ros2 topic pub --once /rb_nexus/digital/output std_msgs/msg/Int32 "{data: 5}"
const uint8_t INPUT_PIN = RB_PIN_D4;
const uint8_t OUTPUT_PINS[] = {RB_PIN_D12, RB_PIN_D14, RB_PIN_D26, RB_PIN_D27};
rcl_publisher_t inputPub;
rcl_subscription_t outputSub;
std_msgs__msg__Int32 input;
std_msgs__msg__Int32 output;

void onOutput(const void* message) {
  const int32_t bits = ((const std_msgs__msg__Int32*)message)->data;
  for (uint8_t i = 0; i < 4; ++i) {
    if (bits & (1 << i)) RB.pinOn(OUTPUT_PINS[i]);
    else RB.pinOff(OUTPUT_PINS[i]);
  }
}

bool createROS() {
  return RBROS.publisher(inputPub, RB_MSG(std_msgs, Int32), RB_TOPIC_DIGITAL_IN) &&
         RBROS.subscriber(outputSub, RB_MSG(std_msgs, Int32), RB_TOPIC_DIGITAL_OUT, &output, onOutput);
}

void setup() {
  RB.begin();
  pinMode(INPUT_PIN, INPUT_PULLUP);
  for (uint8_t pin : OUTPUT_PINS) RB.pinOff(pin);
  RBROS.begin("rb_nexus_digital_io", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 100) {
    last = millis();
    input.data = RB.pinRead(INPUT_PIN) ? 1 : 0;
    RBROS.publish(inputPub, &input);
  }
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Battery voltage from A8 through an 11:1 divider (e.g. 10k + 1k), 1 Hz.
// Adjust DIVIDER and the 3S Li-ion limits (10.0 .. 12.6 V) for your battery.
const float DIVIDER = 11.0f;
rcl_publisher_t batteryPub;
sensor_msgs__msg__BatteryState battery;

bool createROS() {
  return RBROS.publisher(batteryPub, RB_MSG(sensor_msgs, BatteryState), RB_TOPIC_BATTERY);
}

void setup() {
  RB.begin();
  battery.present = true;
  battery.current = NAN;   // Not measured
  battery.charge = NAN;
  battery.capacity = NAN;
  battery.design_capacity = NAN;
  battery.temperature = NAN;
  battery.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_UNKNOWN;
  RBROS.begin("rb_nexus_battery_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 1000) {
    last = millis();
    battery.voltage = RB.analogVolt(8) * DIVIDER;
    battery.percentage = constrain((battery.voltage - 10.0f) / (12.6f - 10.0f), 0.0f, 1.0f);
    RBROS.stamp(battery.header.stamp);
    RBROS.publish(batteryPub, &battery);
  }
}

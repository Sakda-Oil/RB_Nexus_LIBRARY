#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// Start the agent (before or after the board):
//   ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
// The board reconnects automatically. Do not print to Serial: it carries ROS data.

// Publishes raw IMU data (accel m/s^2, gyro rad/s) on /rb_nexus/imu at 50 Hz.
rcl_publisher_t imuPub;
sensor_msgs__msg__Imu imu;
char imuFrame[] = "imu_link";

bool createROS() {
  return RBROS.publisher(imuPub, RB_MSG(sensor_msgs, Imu), RB_TOPIC_IMU_RAW);
}

void setup() {
  RB.begin();
  RB.imuBegin();
  imu.header.frame_id.data = imuFrame;
  imu.header.frame_id.size = sizeof(imuFrame) - 1;
  imu.header.frame_id.capacity = sizeof(imuFrame);
  RBROS.begin("rb_nexus_imu_pub", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 20) {
    last = millis();
    if (RBROS.fillImu(imu)) RBROS.publish(imuPub, &imu);
  }
}

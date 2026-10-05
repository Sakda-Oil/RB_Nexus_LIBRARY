#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t publisher;
sensor_msgs__msg__Imu msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  RB.begin();
  RB.imuBegin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "rb_nexus_imu_pub", "", &support);
  rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
    RB_TOPIC_IMU_RAW);

  static char frame_id[] = "imu_link";
  msg.frame_id.data = frame_id;
  msg.frame_id.size = strlen(frame_id);
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 20) { // 50 Hz
    lastPub = millis();
    msg.linear_acceleration.x = RB.accelX();
    msg.linear_acceleration.y = RB.accelY();
    msg.linear_acceleration.z = RB.accelZ();
    msg.angular_velocity.x = RB.gyroX();
    msg.angular_velocity.y = RB.gyroY();
    msg.angular_velocity.z = RB.gyroZ();
    rcl_publish(&publisher, &msg, NULL);
  }
}

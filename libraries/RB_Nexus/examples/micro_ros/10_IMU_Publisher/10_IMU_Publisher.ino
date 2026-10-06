#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

rcl_publisher_t publisher;
sensor_msgs__msg__Imu msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

void setup() {
  Serial.begin(115200);
  RB.begin();
  RB.imuBegin();

  set_microros_transports();
  allocator = rcutils_get_default_allocator();
  rbROSCheck(rclc_support_init(&support, 0, NULL, &allocator));
  rbROSCheck(rclc_node_init_default(&node, "rb_nexus_imu_pub", "", &support));
  rbROSCheck(rclc_publisher_init_default(
    &publisher, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
    RB_TOPIC_IMU_RAW));

  static char frame_id[] = "imu_link";
  msg.header.frame_id.data = frame_id;
  msg.header.frame_id.size = strlen(frame_id);
  msg.header.frame_id.capacity = sizeof(frame_id);
  rmw_uros_sync_session(1000);
}

void loop() {
  RB.update();
  static unsigned long lastPub = 0;
  if (millis() - lastPub >= 20) { // 50 Hz
    lastPub = millis();
    if (!rbFillImuMessage(msg)) return;
    rcl_publish(&publisher, &msg, NULL);
  }
}

#include <RB_Nexus.h>
#include <RB_Nexus_MicroROS.h>

// micro-ROS over Wi-Fi UDP. Start the agent on the ROS 2 computer:
//   ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
// The board reconnects automatically if Wi-Fi or the agent drops.
const char* WIFI_SSID = "Your_WiFi_SSID";
const char* WIFI_PASS = "Your_WiFi_Password";
IPAddress agentIP(192, 168, 1, 100); // IP address of the computer running the agent
const uint16_t AGENT_PORT = 8888;

rcl_publisher_t counterPub;
std_msgs__msg__Int32 counter;

bool createROS() {
  return RBROS.publisher(counterPub, RB_MSG(std_msgs, Int32), "/rb_nexus/wifi_test");
}

void setup() {
  Serial.begin(115200);  // Free for debug text: ROS data goes over Wi-Fi
  RB.begin();
  if (!RBROS.useWiFi(WIFI_SSID, WIFI_PASS, agentIP, AGENT_PORT)) {
    Serial.println("Wi-Fi connection failed: check SSID and password, then reset.");
    while (true) { RB.ledToggle(); delay(100); }
  }
  Serial.printf("Wi-Fi OK, IP %s. Waiting for agent...\n", RB.wifiIP().c_str());
  RBROS.begin("rb_nexus_wifi_node", createROS);
}

void loop() {
  RB.update();
  RBROS.spin();

  static RBROSState lastState = RBROSState::WaitingAgent;
  if (RBROS.state() != lastState) {
    lastState = RBROS.state();
    Serial.printf("micro-ROS: %s\n", RBROS.stateText());
  }

  static uint32_t last = 0;
  if (RBROS.connected() && millis() - last >= 1000) {
    last = millis();
    counter.data++;
    RBROS.publish(counterPub, &counter);
    RB.ledToggle();
  }
}

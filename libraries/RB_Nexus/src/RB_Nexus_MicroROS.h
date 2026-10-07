// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file RB_Nexus_MicroROS.h
 * @brief micro-ROS Integration, Standard Topics, State Machine, and Watchdog for RB_Nexus.
 *
 * Designed for ROS 2 Humble (Ubuntu 22.04) and ROS 2 Jazzy (Ubuntu 24.04).
 */

#pragma once
#include <Arduino.h>
#include "RB_Nexus.h"
#include "RB_Nexus_Safety.h"

// Real micro-ROS types only: a missing dependency must never look like success.
#if !__has_include(<micro_ros_arduino.h>)
#error "RB_Nexus: Missing micro_ros_arduino. Install the tested Jazzy snapshot using scripts/install_dependencies.py; Library Manager versions may target a different ROS distribution."
#endif
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/int16.h>
#include <std_msgs/msg/bool.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/int32_multi_array.h>
#include <sensor_msgs/msg/imu.h>
#include <sensor_msgs/msg/battery_state.h>
#define RB_HAS_MICROROS_LIB 1

// Stop on initialization failure. Do not print debug text on the ROS Serial port.
inline void rbROSCheck(rcl_ret_t result) {
  if (result == RCL_RET_OK) return;
  RB.emergencyStop();
  while (true) { RB.toggleLED(); delay(100); }
}

inline bool rbFillImuMessage(sensor_msgs__msg__Imu& msg) {
  if (!RB.imuDataFresh()) return false;
  msg.linear_acceleration.x = RB.accelX();
  msg.linear_acceleration.y = RB.accelY();
  msg.linear_acceleration.z = RB.accelZ();
  msg.angular_velocity.x = RB.gyroX();
  msg.angular_velocity.y = RB.gyroY();
  msg.angular_velocity.z = RB.gyroZ();
  // Raw IMU topic: vendor fusion frames differ. Do not label them ROS ENU.
  msg.orientation.w = 1;
  msg.orientation.x = msg.orientation.y = msg.orientation.z = 0;
  msg.orientation_covariance[0] = -1;
  if (rmw_uros_epoch_synchronized()) {
    int64_t ns = rmw_uros_epoch_nanos();
    msg.header.stamp.sec = ns / 1000000000LL;
    msg.header.stamp.nanosec = ns % 1000000000LL;
  }
  return true;
}

// =============================================================================
// Standard ROS 2 Topic Names
// =============================================================================
#define RB_TOPIC_ANALOG_MULTI      "/rb_nexus/analog"
#define RB_TOPIC_ANALOG_CH1        "/rb_nexus/analog/1"
#define RB_TOPIC_ENCODER_CH1       "/rb_nexus/encoder/1"
#define RB_TOPIC_ENCODER_CH2       "/rb_nexus/encoder/2"
#define RB_TOPIC_ENCODER_CH3       "/rb_nexus/encoder/3"
#define RB_TOPIC_ENCODER_CH4       "/rb_nexus/encoder/4"
#define RB_TOPIC_IMU_RAW           "/rb_nexus/imu"
#define RB_TOPIC_BATTERY           "/rb_nexus/battery"
#define RB_TOPIC_MOTOR_CMD         "/rb_nexus/motor/cmd"
#define RB_TOPIC_MOTOR_1_CMD       "/rb_nexus/motor/1/cmd"
#define RB_TOPIC_SERVO_1_CMD       "/rb_nexus/servo/1/cmd"
#define RB_TOPIC_PWM_CMD           "/rb_nexus/pwm/cmd"
#define RB_TOPIC_DIGITAL_IN        "/rb_nexus/digital/input"
#define RB_TOPIC_DIGITAL_OUT       "/rb_nexus/digital/output"
#define RB_TOPIC_LED_CMD           "/rb_nexus/led/cmd"
#define RB_TOPIC_BUTTON_STATE      "/rb_nexus/button/state"
#define RB_TOPIC_ROBOT_STATUS      "/rb_nexus/status"

// =============================================================================
// micro-ROS Agent Connection State Machine
// =============================================================================
enum RBMicroROSState {
  RB_UROS_WAITING_AGENT,
  RB_UROS_AGENT_AVAILABLE,
  RB_UROS_CONNECTED,
  RB_UROS_DISCONNECTED
};

enum RBMicroROSTransport {
  RB_TRANSPORT_SERIAL,
  RB_TRANSPORT_WIFI_UDP
};

class RBNexusMicroROS {
public:
  RBNexusMicroROS()
    : _state(RB_UROS_WAITING_AGENT),
      _transport(RB_TRANSPORT_SERIAL),
      _lastPingTime(0),
      _pingIntervalMs(1000),
      _agentLostTimeoutMs(3000),
      _safetyStopOnDisconnect(true) {}

  void beginSerial(unsigned long baud = 115200) {
    _transport = RB_TRANSPORT_SERIAL;
    Serial.begin(baud);
    rmw_uros_set_custom_transport(true, &Serial,
      [](uxrCustomTransport*) -> bool { return true; },
      [](uxrCustomTransport*) -> bool { return true; },
      [](uxrCustomTransport*, const uint8_t* data, size_t len, uint8_t* err) -> size_t {
        *err = 0; return Serial.write(data, len);
      },
      [](uxrCustomTransport*, uint8_t* data, size_t len, int timeout, uint8_t* err) -> size_t {
        *err = 0; Serial.setTimeout(timeout); return Serial.readBytes(data, len);
      });
    _state = RB_UROS_WAITING_AGENT;
  }

  bool beginWiFi(const char* ssid, const char* pass, IPAddress agentIP, uint16_t agentPort = 8888) {
    _transport = RB_TRANSPORT_WIFI_UDP;
    _state = RB_UROS_WAITING_AGENT;
    if (!RB.wifiConnect(ssid, pass)) return false;
    _locator.address = agentIP;
    _locator.port = agentPort;
    rmw_uros_set_custom_transport(false, &_locator,
      arduino_wifi_transport_open, arduino_wifi_transport_close,
      arduino_wifi_transport_write, arduino_wifi_transport_read);
    return true;
  }

  void setSafetyStopOnDisconnect(bool enable) {
    _safetyStopOnDisconnect = enable;
  }

  RBMicroROSState getState() const { return _state; }

  const char* getStateString() const {
    switch (_state) {
      case RB_UROS_WAITING_AGENT:    return "WAITING_AGENT";
      case RB_UROS_AGENT_AVAILABLE:  return "AGENT_AVAILABLE";
      case RB_UROS_CONNECTED:        return "CONNECTED";
      case RB_UROS_DISCONNECTED:     return "DISCONNECTED";
      default:                       return "UNKNOWN";
    }
  }

  void updateConnection(bool agentAlive) {
    unsigned long now = millis();
    if (agentAlive) {
      _lastPingTime = now;
      if (_state != RB_UROS_CONNECTED) {
        _state = RB_UROS_CONNECTED;
      }
    } else {
      if (_state == RB_UROS_CONNECTED && (now - _lastPingTime > _agentLostTimeoutMs)) {
        _state = RB_UROS_DISCONNECTED;
        if (_safetyStopOnDisconnect) {
          RB.emergencyStop();
        }
      }
    }
  }

private:
  micro_ros_agent_locator _locator;
  RBMicroROSState     _state;
  RBMicroROSTransport _transport;
  unsigned long       _lastPingTime;
  unsigned long       _pingIntervalMs;
  unsigned long       _agentLostTimeoutMs;
  bool                _safetyStopOnDisconnect;
};

inline RBNexusMicroROS RBMicroROS;

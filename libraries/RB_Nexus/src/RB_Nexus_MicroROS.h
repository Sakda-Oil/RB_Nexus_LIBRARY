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

// Check if external micro_ros_arduino library is present in include path
#if __has_include(<micro_ros_arduino.h>)
  #include <micro_ros_arduino.h>
  #include <stdio.h>
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
#else
  #define RB_HAS_MICROROS_LIB 0

  // Fallback type definitions to ensure clean compilation without external library
  typedef struct { int dummy; } rcl_allocator_t;
  typedef struct { int dummy; } rcl_context_t;
  typedef struct { rcl_context_t context; int dummy; } rclc_support_t;
  typedef struct { int dummy; } rcl_node_t;
  typedef struct { int dummy; } rcl_publisher_t;
  typedef struct { int dummy; } rcl_subscription_t;
  typedef struct { int dummy; } rcl_timer_t;
  typedef struct { int dummy; } rclc_executor_t;
  typedef struct { int dummy; } rosidl_message_type_support_t;

  typedef struct { int32_t data; } std_msgs__msg__Int32;
  typedef struct { int16_t data; } std_msgs__msg__Int16;
  typedef struct { bool data; } std_msgs__msg__Bool;
  typedef struct { float data; } std_msgs__msg__Float32;
  typedef struct {
    struct { size_t size; size_t capacity; int32_t* data; } data;
  } std_msgs__msg__Int32MultiArray;

  typedef struct {
    struct { char* data; size_t size; } frame_id;
    struct { double x, y, z, w; } orientation;
    struct { double x, y, z; } angular_velocity;
    struct { double x, y, z; } linear_acceleration;
  } sensor_msgs__msg__Imu;

  typedef struct {
    float voltage;
    float current;
    float charge;
    float capacity;
    float percentage;
    uint8_t power_supply_status;
  } sensor_msgs__msg__BatteryState;

  inline rcl_allocator_t rcutils_get_default_allocator() { rcl_allocator_t a = {0}; return a; }
  inline int rclc_support_init(rclc_support_t*, int, char**, rcl_allocator_t*) { return 0; }
  inline int rclc_node_init_default(rcl_node_t*, const char*, const char*, rclc_support_t*) { return 0; }
  #define ROSIDL_GET_MSG_TYPE_SUPPORT(pkg, subfolder, msg_name) ((const rosidl_message_type_support_t*)0)
  inline int rclc_publisher_init_default(rcl_publisher_t*, const rcl_node_t*, const rosidl_message_type_support_t*, const char*) { return 0; }
  inline int rclc_subscription_init_default(rcl_subscription_t*, const rcl_node_t*, const rosidl_message_type_support_t*, const char*) { return 0; }
  inline int rcl_publish(const rcl_publisher_t*, const void*, void*) { return 0; }
  inline int rclc_executor_init(rclc_executor_t*, void*, size_t, rcl_allocator_t*) { return 0; }
  inline int rclc_executor_add_subscription(rclc_executor_t*, rcl_subscription_t*, void*, void (*)(const void*), int) { return 0; }
  inline int rclc_executor_spin_some(rclc_executor_t*, uint64_t) { return 0; }
  inline void set_microros_transports() {}
  inline void set_microros_wifi_transports(char*, char*, IPAddress, int) {}
  #define RCSOFTCHECK(fn) (fn)
  #define RCCHECK(fn) (fn)
  #define ON_NEW_DATA 0
#endif

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
    _state = RB_UROS_WAITING_AGENT;
  }

  void beginWiFi(const char* ssid, const char* pass, IPAddress agentIP, uint16_t agentPort = 8888) {
    _transport = RB_TRANSPORT_WIFI_UDP;
    RB.wifiConnect(ssid, pass);
    _state = RB_UROS_WAITING_AGENT;
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
  RBMicroROSState     _state;
  RBMicroROSTransport _transport;
  unsigned long       _lastPingTime;
  unsigned long       _pingIntervalMs;
  unsigned long       _agentLostTimeoutMs;
  bool                _safetyStopOnDisconnect;
};

extern RBNexusMicroROS RBMicroROS;

// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file RB_Nexus_MicroROS.h
 * @brief micro-ROS for RB_Nexus: automatic agent connect/reconnect, short helpers.
 *
 * Designed for ROS 2 Humble (Ubuntu 22.04) and ROS 2 Jazzy (Ubuntu 24.04).
 *
 * Usage:
 *   bool createROS() {                  // Create publishers/subscribers here
 *     return RBROS.publisher(pub, RB_MSG(std_msgs, Int32), "/rb_nexus/counter");
 *   }
 *   void setup() { RB.begin(); RBROS.begin("my_node", createROS); }
 *   void loop()  { RB.update(); RBROS.spin(); if (RBROS.connected()) ... }
 *
 * The agent may start before or after the board. When the agent is lost,
 * all motors stop and the board waits for the agent again.
 * Do not print to Serial while using the USB Serial transport.
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
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/int16.h>
#include <std_msgs/msg/bool.h>
#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/int32_multi_array.h>
#include <sensor_msgs/msg/imu.h>
#include <sensor_msgs/msg/battery_state.h>
#define RB_HAS_MICROROS_LIB 1

// Short form of ROSIDL_GET_MSG_TYPE_SUPPORT(package, msg, Type).
#define RB_MSG(package, Type) ROSIDL_GET_MSG_TYPE_SUPPORT(package, msg, Type)

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
#define RB_TOPIC_EMERGENCY_STOP    "/rb_nexus/emergency_stop"

// =============================================================================
// micro-ROS agent connection state
// =============================================================================
enum class RBROSState : uint8_t {
  WaitingAgent, // Pinging the agent every 0.5 s
  Connecting,   // Agent answered: creating node and entities
  Connected,    // Running; agent pinged every 1 s
  Disconnected  // Agent lost: motors stopped, entities destroyed
};

class RBNexusMicroROS {
public:
  static constexpr uint8_t MaxPublishers = 10;
  static constexpr uint8_t MaxSubscribers = 8;
  using CreateFunction = bool (*)();

  // --- Transport (choose one before begin(); USB Serial is the default) ---
  void useSerial(unsigned long baud = 115200) {
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
    _transportReady = true;
  }

  bool useWiFi(const char* ssid, const char* pass, IPAddress agentIP, uint16_t agentPort = 8888) {
    _transportReady = false;
    if (!RB.wifiConnect(ssid, pass)) return false;
    RB.wifiAutoReconnect(true);
    _locator.address = agentIP;
    _locator.port = agentPort;
    rmw_uros_set_custom_transport(false, &_locator,
      arduino_wifi_transport_open, arduino_wifi_transport_close,
      arduino_wifi_transport_write, arduino_wifi_transport_read);
    _transportReady = true;
    return true;
  }

  // --- Start: does not block; connection happens inside spin() ---
  void begin(const char* nodeName, CreateFunction createEntities = nullptr) {
    if (!_transportReady) useSerial();
    _nodeName = nodeName ? nodeName : "rb_nexus";
    _create = createEntities;
    _state = RBROSState::WaitingAgent;
    _lastPing = millis() - 1000;
  }

  // Call every loop(). Handles connect, reconnect and incoming messages.
  void spin() {
    const uint32_t now = millis();
    switch (_state) {
      case RBROSState::WaitingAgent:
        if (uint32_t(now - _lastPing) >= 500) {
          _lastPing = now;
          if (rmw_uros_ping_agent(50, 1) == RMW_RET_OK) _state = RBROSState::Connecting;
        }
        break;
      case RBROSState::Connecting:
        if (createSession()) {
          _state = RBROSState::Connected;
          _lastPing = millis();
        } else {
          destroySession();
          _state = RBROSState::WaitingAgent;
        }
        break;
      case RBROSState::Connected:
        if (uint32_t(now - _lastPing) >= 1000) {
          _lastPing = now;
          if (rmw_uros_ping_agent(50, 3) != RMW_RET_OK) {
            _state = RBROSState::Disconnected;
            break;
          }
        }
        rclc_executor_spin_some(&_executor, RCL_MS_TO_NS(1));
        break;
      case RBROSState::Disconnected:
        if (_stopMotorsWhenLost) RB.stop();
        destroySession();
        _state = RBROSState::WaitingAgent;
        break;
    }
  }

  bool connected() const { return _state == RBROSState::Connected; }
  RBROSState state() const { return _state; }
  const char* stateText() const {
    switch (_state) {
      case RBROSState::WaitingAgent: return "WAITING_AGENT";
      case RBROSState::Connecting:   return "CONNECTING";
      case RBROSState::Connected:    return "CONNECTED";
      case RBROSState::Disconnected: return "DISCONNECTED";
    }
    return "UNKNOWN";
  }
  void stopMotorsWhenLost(bool enable) { _stopMotorsWhenLost = enable; }

  // --- Use inside the create function passed to begin() ---
  bool publisher(rcl_publisher_t& pub, const rosidl_message_type_support_t* type, const char* topic) {
    if (_publisherCount >= MaxPublishers) return false;
    pub = rcl_get_zero_initialized_publisher();
    if (rclc_publisher_init_default(&pub, &_node, type, topic) != RCL_RET_OK) return false;
    _publishers[_publisherCount++] = &pub;
    return true;
  }

  // callback: void myCallback(const void* message)
  bool subscriber(rcl_subscription_t& sub, const rosidl_message_type_support_t* type,
                  const char* topic, void* message, rclc_subscription_callback_t callback) {
    if (_subscriberCount >= MaxSubscribers || !message || !callback) return false;
    sub = rcl_get_zero_initialized_subscription();
    if (rclc_subscription_init_default(&sub, &_node, type, topic) != RCL_RET_OK) return false;
    _subscribers[_subscriberCount++] = &sub;
    return rclc_executor_add_subscription(&_executor, &sub, message, callback, ON_NEW_DATA) == RCL_RET_OK;
  }

  // Publishes only while connected; returns false otherwise.
  bool publish(rcl_publisher_t& pub, const void* message) {
    return connected() && rcl_publish(&pub, message, NULL) == RCL_RET_OK;
  }

  // Fill a raw IMU message (accel m/s^2, gyro rad/s); false when data is stale.
  bool fillImu(sensor_msgs__msg__Imu& msg) {
    if (!RB.imuFresh()) return false;
    msg.linear_acceleration.x = RB.accelX();
    msg.linear_acceleration.y = RB.accelY();
    msg.linear_acceleration.z = RB.accelZ();
    msg.angular_velocity.x = RB.gyroX();
    msg.angular_velocity.y = RB.gyroY();
    msg.angular_velocity.z = RB.gyroZ();
    // Raw IMU topic: vendor fusion frames differ, so orientation is marked unknown.
    msg.orientation.w = 1;
    msg.orientation.x = msg.orientation.y = msg.orientation.z = 0;
    msg.orientation_covariance[0] = -1;
    stamp(msg.header.stamp);
    return true;
  }

  // Agent time when synchronized; left unchanged otherwise.
  void stamp(builtin_interfaces__msg__Time& time) {
    if (!rmw_uros_epoch_synchronized()) return;
    const int64_t ns = rmw_uros_epoch_nanos();
    time.sec = int32_t(ns / 1000000000LL);
    time.nanosec = uint32_t(ns % 1000000000LL);
  }

  // Advanced access for entities the helpers do not cover (timers, services).
  rcl_node_t& node() { return _node; }
  rclc_support_t& support() { return _support; }

private:
  bool createSession() {
    _publisherCount = _subscriberCount = 0;
    _allocator = rcl_get_default_allocator();
    if (rclc_support_init(&_support, 0, NULL, &_allocator) != RCL_RET_OK) return false;
    _supportReady = true;
    _node = rcl_get_zero_initialized_node();
    if (rclc_node_init_default(&_node, _nodeName, "", &_support) != RCL_RET_OK) return false;
    _nodeReady = true;
    _executor = rclc_executor_get_zero_initialized_executor();
    if (rclc_executor_init(&_executor, &_support.context, MaxSubscribers, &_allocator) != RCL_RET_OK) return false;
    _executorReady = true;
    if (_create && !_create()) return false;
    rmw_uros_sync_session(100);
    return true;
  }

  void destroySession() {
    if (_supportReady) {
      rmw_context_t* context = rcl_context_get_rmw_context(&_support.context);
      (void)rmw_uros_set_context_entity_destroy_session_timeout(context, 0);
    }
    if (_nodeReady) {
      for (uint8_t i = 0; i < _publisherCount; ++i) (void)rcl_publisher_fini(_publishers[i], &_node);
      for (uint8_t i = 0; i < _subscriberCount; ++i) (void)rcl_subscription_fini(_subscribers[i], &_node);
    }
    _publisherCount = _subscriberCount = 0;
    if (_executorReady) (void)rclc_executor_fini(&_executor);
    if (_nodeReady) (void)rcl_node_fini(&_node);
    if (_supportReady) (void)rclc_support_fini(&_support);
    _executorReady = _nodeReady = _supportReady = false;
  }

  micro_ros_agent_locator _locator;
  rcl_allocator_t _allocator;
  rclc_support_t _support;
  rcl_node_t _node;
  rclc_executor_t _executor;
  rcl_publisher_t* _publishers[MaxPublishers] = {};
  rcl_subscription_t* _subscribers[MaxSubscribers] = {};
  uint8_t _publisherCount = 0, _subscriberCount = 0;
  bool _supportReady = false, _nodeReady = false, _executorReady = false;
  bool _transportReady = false, _stopMotorsWhenLost = true;
  const char* _nodeName = "rb_nexus";
  CreateFunction _create = nullptr;
  RBROSState _state = RBROSState::WaitingAgent;
  uint32_t _lastPing = 0;
};

inline RBNexusMicroROS RBROS;

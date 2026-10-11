#pragma once
#include <Arduino.h>

// HC-SR04-only prototype has one sensor per vehicle, so this monitor reports
// measurement/link health but does not claim to distinguish an empty scene
// from a failed sensor with certainty. A timeout can mean no echo OR a fault.
enum SensorHealthState : uint8_t {
  SENSOR_HEALTH_OK = 0,
  SENSOR_HEALTH_DEGRADED = 1,
  SENSOR_HEALTH_FAULT_SUSPECTED = 2
};

class SensorHealthMonitor {
 public:
  static constexpr uint8_t FAILURE_THRESHOLD = 5;
  static constexpr uint8_t RECOVERY_THRESHOLD = 5;

  void reportReading(bool measurement_valid) {
    if (measurement_valid) {
      consecutive_failures_ = 0;
      if (state_ == SENSOR_HEALTH_FAULT_SUSPECTED || state_ == SENSOR_HEALTH_DEGRADED) {
        if (consecutive_successes_ < RECOVERY_THRESHOLD) ++consecutive_successes_;
        if (consecutive_successes_ >= RECOVERY_THRESHOLD) {
          state_ = SENSOR_HEALTH_OK;
          consecutive_successes_ = 0;
        }
      } else {
        state_ = SENSOR_HEALTH_OK;
        consecutive_successes_ = 0;
      }
    } else {
      consecutive_successes_ = 0;
      if (consecutive_failures_ < FAILURE_THRESHOLD) ++consecutive_failures_;
      if (consecutive_failures_ >= FAILURE_THRESHOLD) state_ = SENSOR_HEALTH_FAULT_SUSPECTED;
      else if (consecutive_failures_ > 0) state_ = SENSOR_HEALTH_DEGRADED;
    }
  }

  SensorHealthState state() const { return state_; }
  bool faultSuspected() const { return state_ == SENSOR_HEALTH_FAULT_SUSPECTED; }
  uint8_t consecutiveFailures() const { return consecutive_failures_; }

 private:
  SensorHealthState state_ = SENSOR_HEALTH_OK;
  uint8_t consecutive_failures_ = 0;
  uint8_t consecutive_successes_ = 0;
};

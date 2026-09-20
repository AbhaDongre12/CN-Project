#pragma once
#include <Arduino.h>
#include "blindlink_types.h"
#include "sensor_interface.h"

// ============================================================
// MODULE 2: SensorHealthMonitor
// Tracks HEALTHY/DEGRADED/FAILED per sensor and decides which
// sensor's reading to trust this cycle. Failover requires
// several consecutive failures; recovery requires a validation
// period of consecutive good readings — never flips on one
// bad/good sample (addendum §12 hysteresis requirement).
// ============================================================

#define FAIL_THRESHOLD        5     // consecutive bad reads -> FAILED
#define RECOVERY_VALIDATION_N 5     // consecutive good reads before switching back

class SensorHealthMonitor {
 public:
  SensorHealth primaryHealth()  const { return primary_health_; }
  SensorHealth backupHealth()   const { return backup_health_; }
  bool usingBackup()            const { return using_backup_; }

  // Feed every primary reading attempt (valid=false counts as a failure).
  void reportPrimary(bool valid) {
    if (valid) {
      primary_fail_streak_ = 0;
      if (primary_health_ == SENSOR_FAILED) {
        primary_recovery_streak_++;
        if (primary_recovery_streak_ >= RECOVERY_VALIDATION_N) {
          primary_health_ = SENSOR_HEALTHY;
          using_backup_ = false;
          primary_recovery_streak_ = 0;
        }
      } else {
        primary_health_ = SENSOR_HEALTHY;
      }
    } else {
      primary_recovery_streak_ = 0;
      primary_fail_streak_++;
      if (primary_fail_streak_ >= FAIL_THRESHOLD && primary_health_ != SENSOR_FAILED) {
        primary_health_ = SENSOR_FAILED;
        using_backup_ = (backup_health_ != SENSOR_FAILED);
      }
    }
  }

  void reportBackup(bool valid) {
    if (valid) {
      backup_fail_streak_ = 0;
      if (backup_health_ == SENSOR_FAILED) backup_health_ = SENSOR_DEGRADED_H;
      else backup_health_ = SENSOR_HEALTHY;
    } else {
      backup_fail_streak_++;
      if (backup_fail_streak_ >= FAIL_THRESHOLD) backup_health_ = SENSOR_FAILED;
    }
  }

  // Overall node sensing state for NodeStateMachine to consume.
  NodeState overallState() const {
    if (primary_health_ == SENSOR_FAILED && backup_health_ == SENSOR_FAILED)
      return NODE_SENSOR_FAILURE;
    if (primary_health_ != SENSOR_HEALTHY) return NODE_DEGRADED;
    return NODE_READY;
  }

 private:
  SensorHealth primary_health_ = SENSOR_HEALTHY;
  SensorHealth backup_health_  = SENSOR_HEALTHY;
  bool using_backup_ = false;
  uint8_t primary_fail_streak_ = 0;
  uint8_t primary_recovery_streak_ = 0;
  uint8_t backup_fail_streak_ = 0;
};

#pragma once
#include <Arduino.h>
#include "shared_types.h"

// Person 1: HC-SR04-only interface for the agreed hardware.
// `valid` means this sensor returned a plausible echo measurement.
// `object_present` means the measured reflecting surface is inside the
// configured detection range. HC-SR04 cannot identify the object type.
struct SensorReading {
  bool valid = false;
  bool object_present = false;
  uint16_t distance_cm = 0;
  Zone zone = ZONE_LEFT;
  bool from_backup = false; // retained for source compatibility; always false here
};

class SensorInterface {
 public:
  SensorInterface()
      : trig_(255), echo_(255), zone_(ZONE_LEFT), detection_max_cm_(250) {}

  SensorInterface(uint8_t trig_pin, uint8_t echo_pin, Zone covered_zone,
                  uint16_t detection_max_cm = 250)
      : trig_(trig_pin), echo_(echo_pin), zone_(covered_zone),
        detection_max_cm_(detection_max_cm) {}

  void configure(uint8_t trig_pin, uint8_t echo_pin, Zone covered_zone,
                 uint16_t detection_max_cm = 250) {
    trig_ = trig_pin;
    echo_ = echo_pin;
    zone_ = covered_zone;
    detection_max_cm_ = detection_max_cm;
    history_[0] = history_[1] = history_[2] = 0;
    history_count_ = history_idx_ = 0;
  }

  void begin() {
    pinMode(trig_, OUTPUT);
    pinMode(echo_, INPUT);
    digitalWrite(trig_, LOW);
    delay(50);
  }

  // One measurement attempt. Call no faster than about every 60 ms.
  // Returns true when a complete, plausible echo was measured. No echo is
  // reported as invalid, NOT as confirmed object disappearance.
  bool read(SensorReading* out) {
    if (out == nullptr) return false;
    *out = SensorReading();
    out->zone = zone_;
    out->from_backup = false;

    digitalWrite(trig_, LOW);
    delayMicroseconds(2);
    digitalWrite(trig_, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_, LOW);

    // 25 ms timeout is approximately 4.3 m round-trip range.
    const unsigned long duration_us = pulseIn(echo_, HIGH, 25000UL);
    if (duration_us == 0) {
      out->valid = false;
      out->object_present = false;
      return false;
    }

    const uint16_t raw_cm = static_cast<uint16_t>((duration_us + 29UL) / 58UL);
    if (raw_cm < MIN_DISTANCE_CM || raw_cm > MAX_SENSOR_DISTANCE_CM) {
      out->valid = false;
      out->object_present = false;
      return false;
    }

    const uint16_t filtered_cm = medianFilter(raw_cm);
    out->valid = true;
    out->distance_cm = filtered_cm;
    out->object_present = (filtered_cm <= detection_max_cm_);
    return true;
  }

  // Kept as a compatibility wrapper for code that previously called
  // readPrimary(). There is no separate radar/backup sensor in this build.
  bool readPrimary(SensorReading* out) { return read(out); }

  static constexpr uint16_t MIN_DISTANCE_CM = 2;
  static constexpr uint16_t MAX_SENSOR_DISTANCE_CM = 400;

 private:
  uint8_t trig_;
  uint8_t echo_;
  Zone zone_;
  uint16_t detection_max_cm_;
  uint16_t history_[3] = {0, 0, 0};
  uint8_t history_count_ = 0;
  uint8_t history_idx_ = 0;

  uint16_t medianFilter(uint16_t value) {
    history_[history_idx_] = value;
    history_idx_ = (history_idx_ + 1) % 3;
    if (history_count_ < 3) history_count_++;
    // Avoid zero-filled history corrupting the first two readings.
    if (history_count_ == 1) return history_[0];
    if (history_count_ == 2) {
      return (history_[0] + history_[1]) / 2;
    }
    uint16_t a = history_[0], b = history_[1], c = history_[2];
    if (a > b) { uint16_t t = a; a = b; b = t; }
    if (b > c) { uint16_t t = b; b = c; c = t; }
    if (a > b) { uint16_t t = a; a = b; b = t; }
    return b;
  }
};

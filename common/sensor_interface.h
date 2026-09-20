#pragma once
#include <Arduino.h>
#include "shared_types.h"

// ============================================================
// PERSON 1 — SensorInterface
//
// Owns raw sensor reading only. Produces a validated, filtered
// SensorReading — does NOT decide object identity (ObjectTracker's
// job) and does NOT decide NEW/CHANGED/UNCHANGED (Person 2's job).
//
// ZONE CONFIGURATION NOTE (read before wiring up multiple nodes):
// This prototype is single-sensor-per-node, so the zone a node
// covers is a fixed configuration value set at construction time,
// not derived from sensor angle/beam data. If your demo mounts one
// ESP32+LD2410 covering the truck's left blind spot, construct with
// ZONE_LEFT. This matches the "predefined lookup table for a
// single-beam configuration" approach — a real multi-pod system
// would derive zone from which pod reported the detection, which is
// out of scope for this prototype (single truck node, no separate
// pods).
// ============================================================

struct SensorReading {
  bool     valid = false;
  bool     object_present = false;
  uint16_t distance_cm = 0;
  Zone     zone = ZONE_LEFT;      // stamped from this sensor's configured zone
  bool     from_backup = false;   // true if this reading came from the ultrasonic backup
};

class SensorInterface {
 public:
  SensorInterface(HardwareSerial& ld2410_uart, uint8_t ultrasonic_trig_pin,
                   uint8_t ultrasonic_echo_pin, Zone covered_zone)
      : uart_(ld2410_uart), trig_(ultrasonic_trig_pin), echo_(ultrasonic_echo_pin),
        zone_(covered_zone) {}

  void begin() {
    uart_.begin(256000); // LD2410 default baud
    pinMode(trig_, OUTPUT);
    pinMode(echo_, INPUT);
    digitalWrite(trig_, LOW);
  }

  // Call every loop(). Returns true only on cycles where a new LD2410
  // frame was actually parsed (it streams continuously but not every
  // loop() iteration will have a complete frame ready).
  bool readPrimary(SensorReading* out) {
    if (!parseLd2410Frame(out)) return false;
    out->zone = zone_;
    out->from_backup = false;
    return true;
  }

  // Ultrasonic is request/response — poll on a timer (e.g. every 120ms),
  // not every loop(). See PERSON1_README.md for the recommended cadence.
  SensorReading readBackup() {
    SensorReading r;
    r.zone = zone_;
    r.from_backup = true;
    digitalWrite(trig_, LOW); delayMicroseconds(2);
    digitalWrite(trig_, HIGH); delayMicroseconds(10);
    digitalWrite(trig_, LOW);
    unsigned long duration_us = pulseIn(echo_, HIGH, 25000UL); // ~4m timeout window
    if (duration_us == 0) { r.valid = false; return r; }
    uint16_t cm = (uint16_t)(duration_us / 58);
    if (!plausibleDistance(cm)) { r.valid = false; return r; }
    r.valid = true;
    r.object_present = true;
    r.distance_cm = medianFilter(cm);
    return r;
  }

 private:
  HardwareSerial& uart_;
  uint8_t trig_, echo_;
  Zone zone_;
  uint16_t history_[3] = {0, 0, 0};
  uint8_t history_idx_ = 0;

  static bool plausibleDistance(uint16_t cm) {
    return cm >= 10 && cm <= 600; // range-validation gate (spec §11)
  }

  uint16_t medianFilter(uint16_t new_val) {
    history_[history_idx_] = new_val;
    history_idx_ = (history_idx_ + 1) % 3;
    uint16_t a = history_[0], b = history_[1], c = history_[2];
    if (a > b) { uint16_t t = a; a = b; b = t; }
    if (b > c) { uint16_t t = b; b = c; c = t; }
    if (a > b) { uint16_t t = a; a = b; b = t; }
    return b;
  }

  // Minimal-viable LD2410 basic-frame parser. VERIFY byte offsets against
  // a real serial capture from your specific module/firmware before
  // trusting this — see PERSON1_README.md "Before you trust this parser".
  bool parseLd2410Frame(SensorReading* out) {
    static uint8_t buf[64];
    static uint8_t idx = 0;
    while (uart_.available()) {
      uint8_t b = uart_.read();
      if (idx == 0 && b != 0xFD) continue; // sync to frame header
      buf[idx++] = b;
      if (idx >= sizeof(buf)) idx = 0; // overrun guard
      if (idx >= 13 && buf[6] == 0x02) { // 0x02 = basic-mode report id (typical)
        uint8_t target_state = buf[8];
        uint16_t moving_distance = buf[9] | (buf[10] << 8);
        out->valid = true;
        out->object_present = (target_state != 0x00);
        out->distance_cm = plausibleDistance(moving_distance) ? moving_distance : 0;
        idx = 0;
        return true;
      }
    }
    return false;
  }
};

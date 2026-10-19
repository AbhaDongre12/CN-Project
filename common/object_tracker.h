
#pragma once

#include <Arduino.h>
#include "shared_types.h"
#include "sensor_interface.h"

// Person 1 implementation. Public interface remains compatible
// with shared_types.h and Person 2's accessors.
class ObjectTracker {
 public:
  ObjectTracker();

  void update(const SensorReading& reading, unsigned long now_ms);

  uint8_t getActiveObjects(TrackedObject out[MAX_OBJECTS]);

  // Returns expired IDs without freeing their tracker slots.
  // The caller must confirm that each ID has been safely queued.
  uint8_t getExpiredObjects(uint8_t out_ids[MAX_OBJECTS]);

  // Release an expired slot only after its CLEAR ID is safely
  // stored in the sender's pending-clear queue.
  bool confirmExpiredObjectQueued(uint8_t object_id);

 private:
  static constexpr uint16_t MATCH_TOLERANCE_CM = 60;
  static constexpr unsigned long GRACE_START_MS = 800;
  static constexpr unsigned long GRACE_DURATION_MS = 2500;

  struct Slot {
    bool in_use = false;
    uint8_t object_id = 0;
    uint16_t distance_cm = 0;
    Zone zone = ZONE_LEFT;
    unsigned long last_seen_ms = 0;
    unsigned long unconfirmed_since_ms = 0;
    bool pending_expire_unreported = false;
  };

  Slot slots_[MAX_OBJECTS];
  uint8_t next_id_ = 1;

  Slot* findMatch(uint16_t distance_cm, Zone zone);

  void allocate(
      uint16_t distance_cm,
      Zone zone,
      unsigned long now_ms);

  void ageSlots(unsigned long now_ms);
};

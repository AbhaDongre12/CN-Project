#pragma once
#include "shared_types.h"
#include "sensor_interface.h"

// ============================================================
// PERSON 1 — ObjectTracker
//
// Implements the ObjectTracker class DECLARED in shared_types.h.
// Internally keeps a richer bookkeeping struct (timestamps, grace-
// window state) than the frozen TrackedObject the outside world
// sees — that's fine, only the public methods and TrackedObject
// itself are the frozen contract. This file, and everything
// private in it, is entirely Person 1's to change.
//
// Contract reminder (see shared_types.h for full comments):
//   - getActiveObjects() / getExpiredObjects() are called as a
//     matched pair, once per agreed processing cycle, by Person 2.
//   - distance_cm/zone are held frozen at last-known values while
//     an object is in its unconfirmed grace window — this is what
//     makes Person 2's diff-against-Local-State-Table logic need
//     no special case for "object temporarily missing."
// ============================================================

class ObjectTracker {
 public:
  ObjectTracker() { for (auto& s : slots_) s = Slot(); }

  // NOT part of the frozen interface — this is Person 1's own driver
  // method, called once per sensor read (which may be more frequent
  // than the getActiveObjects/getExpiredObjects processing cycle).
  // Matches a reading to an existing tracked object by distance
  // proximity within the same zone, or allocates a new slot.
  void update(const SensorReading& r, unsigned long now_ms) {
    if (!r.valid) return;

    if (!r.object_present) {
      // Sensor says nothing detected right now. Do NOT immediately
      // clear anything here — aging into the grace window and
      // eventual expiry is handled uniformly in ageSlots(), called
      // from getActiveObjects() below, so there's exactly one place
      // that logic lives.
      return;
    }

    Slot* match = findMatch(r.distance_cm, r.zone);
    if (match) {
      match->distance_cm = r.distance_cm;
      match->zone = r.zone;
      match->last_seen_ms = now_ms;
      match->unconfirmed_since_ms = 0;
      return;
    }
    allocate(r.distance_cm, r.zone, now_ms);
  }

  // ---- Frozen interface (must match shared_types.h exactly) ----

  uint8_t getActiveObjects(TrackedObject out[MAX_OBJECTS]) {
    ageSlots(millis());
    uint8_t n = 0;
    for (auto& s : slots_) {
      if (!s.in_use || s.pending_expire_unreported) continue;
      out[n].object_id = s.object_id;
      out[n].distance_cm = s.distance_cm;
      out[n].zone = s.zone;
      n++;
    }
    return n;
  }

  uint8_t getExpiredObjects(uint8_t out_ids[MAX_OBJECTS]) {
    uint8_t n = 0;
    for (auto& s : slots_) {
      if (s.in_use && s.pending_expire_unreported) {
        out_ids[n++] = s.object_id;
        s.in_use = false; // free the slot now that Person 2 has been told
      }
    }
    return n;
  }

 private:
  // MATCH_TOLERANCE_CM: how close a new reading's distance must be to an
  // existing tracked object to be considered "the same physical object."
  // This is Person 1's own identity-matching threshold — a DIFFERENT
  // concept from whatever change-reporting threshold Person 2 uses in
  // their Change Detection engine. Keep these independent; don't let
  // Person 2's threshold constant leak into this file or vice versa.
  static const uint16_t MATCH_TOLERANCE_CM = 60;
  // How long with no confirming reading before an object enters its
  // grace window, and how long the grace window itself lasts before
  // the object is reported as expired.
  static const unsigned long UNCONFIRMED_GRACE_START_MS = 800;
  static const unsigned long UNCONFIRMED_GRACE_TOTAL_MS = 2500;

  struct Slot {
    bool     in_use = false;
    uint8_t  object_id = 0;
    uint16_t distance_cm = 0;
    Zone     zone = ZONE_LEFT;
    unsigned long last_seen_ms = 0;
    unsigned long unconfirmed_since_ms = 0; // 0 = currently confirmed
    bool     pending_expire_unreported = false; // true for exactly one
                                                  // getExpiredObjects() call
  };

  Slot slots_[MAX_OBJECTS];
  uint8_t next_id_ = 1; // 0 is reserved/invalid per the frozen contract

  Slot* findMatch(uint16_t distance_cm, Zone zone) {
    Slot* best = nullptr;
    uint16_t best_diff = MATCH_TOLERANCE_CM + 1;
    for (auto& s : slots_) {
      if (!s.in_use || s.pending_expire_unreported) continue;
      if (s.zone != zone) continue; // only match within the same zone
      uint16_t diff = (uint16_t)abs((int)s.distance_cm - (int)distance_cm);
      if (diff <= MATCH_TOLERANCE_CM && diff < best_diff) { best = &s; best_diff = diff; }
    }
    return best;
  }

  void allocate(uint16_t distance_cm, Zone zone, unsigned long now_ms) {
    for (auto& s : slots_) {
      if (!s.in_use) {
        s = Slot();
        s.in_use = true;
        s.object_id = next_id_++;
        if (next_id_ == 0) next_id_ = 1; // wrap, never reissue 0
        s.distance_cm = distance_cm;
        s.zone = zone;
        s.last_seen_ms = now_ms;
        return;
      }
    }
    // TABLE_FULL: no free slot. Per the reliability spec, we do not
    // silently overwrite an active object — the new detection is simply
    // not tracked this cycle. Log this at the call site (main loop) if
    // you want it surfaced, since ObjectTracker itself has no logging
    // dependency by design (keeps it unit-testable standalone).
  }

  void ageSlots(unsigned long now_ms) {
    for (auto& s : slots_) {
      if (!s.in_use || s.pending_expire_unreported) continue;
      unsigned long age = now_ms - s.last_seen_ms;
      if (age > UNCONFIRMED_GRACE_START_MS && s.unconfirmed_since_ms == 0) {
        s.unconfirmed_since_ms = now_ms; // enter grace window
      }
      if (s.unconfirmed_since_ms != 0 &&
          (now_ms - s.unconfirmed_since_ms) > UNCONFIRMED_GRACE_TOTAL_MS) {
        s.pending_expire_unreported = true; // will surface via getExpiredObjects()
      }
    }
  }
};

#pragma once
#include <Arduino.h>

// ============================================================
// SHARED_TYPES.H — FROZEN INTERFACE
//
// This file is the contract between Person 1 (SensorInterface +
// ObjectTracker) and Person 2 (LocalStateTable + ChangeDetection
// + MessageComposition). Both of you #include this exact file
// and do not modify it without agreeing on the change together —
// any field/type change here is a breaking change for the other
// person's code.
//
// Person 1 owns: the .cpp/.h implementation that PRODUCES
//                 TrackedObject values via ObjectTracker's methods.
// Person 2 owns: everything that CONSUMES them — nothing in this
//                 file is Person 2's to implement.
// ============================================================

#define MAX_OBJECTS 8   // bounded table size — agreed limit, do not grow without discussion

// ---- Zone enum ----
// Matches the patent's position/zone label. Values are fixed —
// do not renumber, since these values go on the wire later
// (Person 2's MessageComposition sends this byte directly).
enum Zone : uint8_t {
  ZONE_LEFT  = 0,
  ZONE_RIGHT = 1,
  ZONE_REAR  = 2,
  ZONE_FRONT = 3
};

// ---- TrackedObject ----
// The ONLY struct Person 2's ChangeDetection compares against the
// Local State Table. Deliberately does NOT include a "status" or
// "changed" field — status/lifecycle events are conveyed through
// which accessor method returns the object (see below), not
// through a field on the struct itself. This avoids ambiguity
// about who owns "has this changed" (that's Person 2's job, not
// Person 1's).
struct TrackedObject {
  uint8_t  object_id;    // 1..MAX_OBJECTS. Persistent across cycles for the
                          // SAME physical object (Person 1's matching logic
                          // guarantees this). 0 is never valid/used.
  uint16_t distance_cm;   // Current best-known distance, in centimeters.
                          // Valid range: 10-600. During an unconfirmed grace
                          // window (object temporarily not detected but not
                          // yet cleared), this holds the LAST CONFIRMED
                          // value, unchanged — it does not reset to 0 or
                          // any sentinel. This is intentional: it means an
                          // object in its grace window naturally compares
                          // as UNCHANGED against the Local State Table
                          // without Person 2 needing any special-case logic.
  Zone     zone;          // Last determined zone. Also frozen during a
                          // grace window, same reasoning as distance_cm.
};

// ============================================================
// OBJECTTRACKER — INTERFACE (Person 1 implements the .cpp; this
// declaration is what Person 2 codes against)
// ============================================================
class ObjectTracker {
 public:
  // Call exactly once per processing cycle (Person 1 and Person 2 must
  // agree on cycle cadence — e.g. every 200ms — since these two calls
  // are defined as a matched pair for the SAME cycle; do not call
  // getActiveObjects() twice without an intervening getExpiredObjects()).
  //
  // Fills out[] with every object currently tracked — whether freshly
  // confirmed this cycle, updated this cycle, or sitting inside its
  // unconfirmed grace window (see distance_cm/zone note above).
  // Returns the count written (0..MAX_OBJECTS). out[] must have room
  // for MAX_OBJECTS entries.
  //
  // Person 2's ChangeDetection walks this array each cycle and diffs
  // each object_id's distance_cm/zone against what's stored in the
  // Local State Table for that same object_id to decide NEW / CHANGED /
  // UNCHANGED. An object_id NOT present in this array that WAS present
  // last cycle is not itself a signal — wait for getExpiredObjects().
  uint8_t getActiveObjects(TrackedObject out[MAX_OBJECTS]);

  // Call once per cycle, immediately after getActiveObjects(), even if
  // you don't act on an empty result — Person 1's internal bookkeeping
  // does not depend on this call, but Person 2's CLEAR-message logic
  // does need to see it every cycle to avoid missing a clear event.
  //
  // Fills out_ids[] with the object_id(s) that just expired their grace
  // window THIS cycle. For each id returned here, Person 2 must emit a
  // CLEAR message. After this call, that object_id may be reassigned
  // by Person 1 to a different physical object on a future cycle — so
  // Person 2 must not hold onto a stale mapping of object_id -> "the
  // motorcycle from 5 seconds ago" past this point.
  // Returns the count written (0..MAX_OBJECTS).
  uint8_t getExpiredObjects(uint8_t out_ids[MAX_OBJECTS]);
};

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

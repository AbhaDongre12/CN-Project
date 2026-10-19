
#pragma once

#include <Arduino.h>

// Maximum number of simultaneously tracked objects.
#define MAX_OBJECTS 8

// Zone values are transmitted over ESP-NOW.
// Keep the existing values unchanged for compatibility.
enum Zone : uint8_t {
    ZONE_LEFT = 0,
    ZONE_RIGHT = 1,
    ZONE_REAR = 2,
    ZONE_FRONT = 3,

    // Four specific sender sensor positions.
    ZONE_FRONT_LEFT = 4,
    ZONE_FRONT_RIGHT = 5,
    ZONE_REAR_LEFT = 6,
    ZONE_REAR_RIGHT = 7
};

struct TrackedObject {
    uint8_t object_id;
    uint16_t distance_cm;
    Zone zone;
};

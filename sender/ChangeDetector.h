#pragma once
#include "StateTable.h"

enum ChangeType{
    NEW_OBJECT,
    CHANGED_OBJECT,
    UNCHANGED_OBJECT
};

struct ChangeResult{
    ChangeType type;
    uint8_t object_id;
    uint16_t current_distance;
    uint16_t previous_distance;
    Zone current_zone;
    Zone previous_zone;
    bool distance_changed;
    bool zone_changed;
};

class ChangeDetector{
public:
    ChangeDetector(uint16_t threshold_cm);

    ChangeResult detectChange(const TrackedObject& currentObject,StateTable& stateTable);
    void setThreshold(uint16_t threshold_cm);
    uint16_t getThreshold();

private:
    uint16_t threshold_cm;
};
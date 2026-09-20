#pragma once
#include "../common/shared_types.h"

struct StoredState{
    uint8_t object_id;
    uint16_t distance_cm;
    Zone zone;
    bool valid;
};

class StateTable{
public:
    StateTable();

    void initialize();
    int findObject(uint8_t object_id);
    bool addObject(const TrackedObject& object);
    bool updateObject(const TrackedObject& object);
    bool removeObject(uint8_t object_id);
    StoredState getObject(int index);
    void display();
private:
    StoredState table[MAX_OBJECTS];
};
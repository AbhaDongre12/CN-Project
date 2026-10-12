#pragma once
#include "../common/shared_types.h"

struct StoredState
{
    uint8_t object_id;
    uint16_t distance_cm;
    Zone zone;
    bool valid;
};

struct PendingUpdate
{
    bool active;
    uint8_t sequence_number;
    uint8_t retry_count;
};

struct ObjectState
{
    StoredState local_state;
    StoredState confirmed_state;
    PendingUpdate pending;
};

class StateTable
{
public:
    StateTable();

    void initialize();
    int findObject(uint8_t object_id);
    bool addObject(const TrackedObject &object);
    bool updateObject(const TrackedObject &object);
    bool removeObject(uint8_t object_id);
    StoredState getObject(int index);
    StoredState getConfirmedObject(int index);
    ObjectState getObjectState(int index);
    bool updateConfirmedState(uint8_t object_id, const TrackedObject &object);
    bool setPendingUpdate(uint8_t object_id, uint8_t sequence_number);
    bool clearPendingUpdate(uint8_t object_id);
    void display();

private:
    ObjectState table[MAX_OBJECTS];
};

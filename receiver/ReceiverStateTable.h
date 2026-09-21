#pragma once

#include <Arduino.h>
#include "ReceiverProtocol.h"

#define MAX_NEIGHBOR_OBJECTS 10

struct NeighborObject
{
    bool valid;
    uint8_t vehicle_id;
    uint8_t object_id;
    uint16_t distance_cm;
    Zone zone;
    uint8_t last_sequence;
    unsigned long last_updated;
};

class ReceiverStateTable
{
public:
    ReceiverStateTable();

    bool processMessage(
        const BlindLinkMessage &message);

    NeighborObject *findObject(
        uint8_t vehicle_id,
        uint8_t object_id);

    void removeObject(
        uint8_t vehicle_id,
        uint8_t object_id);

    void removeExpired(
        unsigned long timeout_ms);

    void printTable();

    int getActiveObjectCount();

    NeighborObject *getObjectAt(
        int index);

private:
    NeighborObject objects[MAX_NEIGHBOR_OBJECTS];

    bool isNewerSequence(
        uint8_t new_sequence,
        uint8_t old_sequence);

    bool processFull(
        const BlindLinkMessage &message);

    bool processDelta(
        const BlindLinkMessage &message);

    bool processClear(
        const BlindLinkMessage &message);
};
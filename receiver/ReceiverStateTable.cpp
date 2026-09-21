#include "ReceiverStateTable.h"

ReceiverStateTable::ReceiverStateTable()
{
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        objects[i].valid = false;
        objects[i].vehicle_id = 0;
        objects[i].object_id = 0;
        objects[i].distance_cm = 0;
        objects[i].zone = static_cast<Zone>(0);
        objects[i].last_sequence = 0;
        objects[i].last_updated = 0;
    }
}

NeighborObject *ReceiverStateTable::findObject(
    uint8_t vehicle_id,
    uint8_t object_id)
{
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].valid &&
            objects[i].vehicle_id == vehicle_id &&
            objects[i].object_id == object_id)
        {
            return &objects[i];
        }
    }

    return nullptr;
}

bool ReceiverStateTable::isNewerSequence(
    uint8_t new_sequence,
    uint8_t old_sequence)
{
    /*
     * Handles uint8_t sequence number wraparound.
     *
     * Example:
     * old = 255
     * new = 0
     *
     * 0 is treated as newer than 255.
     */

    return (uint8_t)(new_sequence - old_sequence) < 128;
}

bool ReceiverStateTable::processMessage(
    const BlindLinkMessage &message)
{
    switch (message.type)
    {
    case MESSAGE_FULL:
        return processFull(message);

    case MESSAGE_DELTA:
        return processDelta(message);

    case MESSAGE_CLEAR:
        return processClear(message);

    case MESSAGE_HEARTBEAT:
        Serial.println("Heartbeat received.");
        return true;

    case MESSAGE_SUPPRESS:
        Serial.println("Suppress message received.");
        return true;

    default:
        Serial.println("Unknown message type.");
        return false;
    }
}

bool ReceiverStateTable::processFull(
    const BlindLinkMessage &message)
{
    NeighborObject *existing =
        findObject(
            message.vehicle_id,
            message.object_id);

    // Object already exists
    if (existing != nullptr)
    {
        if (!isNewerSequence(
                message.sequence_number,
                existing->last_sequence))
        {
            Serial.println("Old FULL message ignored.");
            return false;
        }

        existing->distance_cm = message.distance_cm;
        existing->zone = message.zone;
        existing->last_sequence = message.sequence_number;
        existing->last_updated = millis();

        Serial.println("Existing object updated from FULL.");
        return true;
    }

    // Find empty slot
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (!objects[i].valid)
        {
            objects[i].valid = true;

            objects[i].vehicle_id = message.vehicle_id;
            objects[i].object_id = message.object_id;
            objects[i].distance_cm = message.distance_cm;
            objects[i].zone = message.zone;
            objects[i].last_sequence = message.sequence_number;
            objects[i].last_updated = millis();

            Serial.println("New object added.");
            return true;
        }
    }

    Serial.println("Receiver state table FULL.");
    return false;
}

bool ReceiverStateTable::processDelta(
    const BlindLinkMessage &message)
{
    NeighborObject *object =
        findObject(
            message.vehicle_id,
            message.object_id);

    // DELTA for an object we don't know
    if (object == nullptr)
    {
        Serial.println("DELTA received for unknown object.");
        return false;
    }

    // Reject old packet
    if (!isNewerSequence(
            message.sequence_number,
            object->last_sequence))
    {
        Serial.println("Old DELTA message ignored.");
        return false;
    }

    // Bit 0 = distance changed
    if (message.changed_fields & 0x01)
    {
        object->distance_cm = message.distance_cm;
    }

    // Bit 1 = zone changed
    if (message.changed_fields & 0x02)
    {
        object->zone = message.zone;
    }

    object->last_sequence = message.sequence_number;
    object->last_updated = millis();

    Serial.println("DELTA applied.");
    return true;
}

bool ReceiverStateTable::processClear(
    const BlindLinkMessage &message)
{
    NeighborObject *object =
        findObject(
            message.vehicle_id,
            message.object_id);

    if (object == nullptr)
    {
        Serial.println("CLEAR received for unknown object.");
        return false;
    }

    object->valid = false;

    Serial.println("Object cleared.");
    return true;
}

void ReceiverStateTable::removeObject(
    uint8_t vehicle_id,
    uint8_t object_id)
{
    NeighborObject *object =
        findObject(vehicle_id, object_id);

    if (object != nullptr)
    {
        object->valid = false;
    }
}

void ReceiverStateTable::removeExpired(
    unsigned long timeout_ms)
{
    unsigned long now = millis();

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].valid &&
            now - objects[i].last_updated > timeout_ms)
        {
            Serial.print("Object timed out: ");
            Serial.println(objects[i].object_id);

            objects[i].valid = false;
        }
    }
}

void ReceiverStateTable::printTable()
{
    Serial.println();
    Serial.println("===== RECEIVER STATE TABLE =====");

    bool found = false;

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (!objects[i].valid)
        {
            continue;
        }

        found = true;

        Serial.print("Vehicle: ");
        Serial.println(objects[i].vehicle_id);

        Serial.print("Object: ");
        Serial.println(objects[i].object_id);

        Serial.print("Distance: ");
        Serial.print(objects[i].distance_cm);
        Serial.println(" cm");

        Serial.print("Zone: ");
        Serial.println((int)objects[i].zone);

        Serial.print("Sequence: ");
        Serial.println(objects[i].last_sequence);

        Serial.println("-----------------------------");
    }

    if (!found)
    {
        Serial.println("No active objects.");
    }

    Serial.println("================================");
}

int ReceiverStateTable::getActiveObjectCount()
{
    int count = 0;

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].valid)
        {
            count++;
        }
    }

    return count;
}

NeighborObject *ReceiverStateTable::getObjectAt(int index)
{
    if (index < 0 || index >= MAX_NEIGHBOR_OBJECTS)
    {
        return nullptr;
    }

    if (!objects[index].valid)
    {
        return nullptr;
    }

    return &objects[index];
}
#include "ReceiverStateTable.h"

ReceiverStateTable::ReceiverStateTable()
{
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        objects[i].valid = false;
        objects[i].vehicle_id = 0;
        objects[i].object_id = 0;
        objects[i].distance_cm = 0;
        objects[i].zone = ZONE_LEFT;
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
    // Sequence comparison supports uint8_t wraparound.
    // The sender skips zero, so 255 -> 1 is considered newer.
    const uint8_t difference =
        static_cast<uint8_t>(new_sequence - old_sequence);

    return difference != 0 && difference < 128;
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
        // ReceiverCommunication performs heartbeat sequence checks.
        Serial.println("Heartbeat received.");
        return true;

    case MESSAGE_SUPPRESS:
        // Local-only message; should not arrive over ESP-NOW.
        Serial.println("SUPPRESS is not a wire message.");
        return false;

    default:
        Serial.println("Unknown message type.");
        return false;
    }
}

bool ReceiverStateTable::processFull(
    const BlindLinkMessage &message)
{
    NeighborObject *existing =
        findObject(message.vehicle_id, message.object_id);

    // Update an active object only if the sequence is newer.
    if (existing != nullptr)
    {
        if (!isNewerSequence(
                message.sequence_number,
                existing->last_sequence))
        {
            Serial.println("Old/duplicate FULL ignored.");
            return false;
        }

        existing->distance_cm = message.distance_cm;
        existing->zone = message.zone;
        existing->last_sequence = message.sequence_number;
        existing->last_updated = millis();

        Serial.println("Existing object updated from FULL.");
        return true;
    }

    // Reuse a cleared or expired slot for this same object, but
    // preserve its previous sequence number to prevent stale FULL
    // messages from resurrecting it.
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (!objects[i].valid &&
            objects[i].vehicle_id == message.vehicle_id &&
            objects[i].object_id == message.object_id &&
            objects[i].last_sequence != 0)
        {
            if (!isNewerSequence(
                    message.sequence_number,
                    objects[i].last_sequence))
            {
                Serial.println(
                    "Old FULL after CLEAR/timeout ignored.");
                return false;
            }

            objects[i].valid = true;
            objects[i].distance_cm = message.distance_cm;
            objects[i].zone = message.zone;
            objects[i].last_sequence = message.sequence_number;
            objects[i].last_updated = millis();

            Serial.println("Object restored from newer FULL.");
            return true;
        }
    }

    // Find a never-used slot.
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (!objects[i].valid &&
            objects[i].last_sequence == 0)
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
        findObject(message.vehicle_id, message.object_id);

    // DELTA cannot create an object. A FULL must establish it first.
    if (object == nullptr)
    {
        Serial.println("DELTA received for unknown object.");
        return false;
    }

    if (!isNewerSequence(
            message.sequence_number,
            object->last_sequence))
    {
        Serial.println("Old/duplicate DELTA ignored.");
        return false;
    }

    // Bit 0: distance changed.
    if (message.changed_fields & 0x01U)
    {
        object->distance_cm = message.distance_cm;
    }

    // Bit 1: zone changed.
    if (message.changed_fields & 0x02U)
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
        findObject(message.vehicle_id, message.object_id);

    if (object == nullptr)
    {
        // We cannot verify this CLEAR against an active object.
        // Do not treat it as a newly processed message.
        Serial.println(
            "CLEAR for unknown/already-cleared object rejected.");
        return false;
    }

    if (!isNewerSequence(
            message.sequence_number,
            object->last_sequence))
    {
        Serial.println("Old/duplicate CLEAR ignored.");
        return false;
    }

    // Keep the identity and last sequence in the slot after clearing.
    // This prevents an older FULL from recreating the object.
    object->last_sequence = message.sequence_number;
    object->last_updated = millis();
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
        // Preserve sequence history.
        object->valid = false;
    }
}

void ReceiverStateTable::removeExpired(
    unsigned long timeout_ms)
{
    const unsigned long now = millis();

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].valid &&
            now - objects[i].last_updated > timeout_ms)
        {
            Serial.print("Object timed out: ");
            Serial.println(objects[i].object_id);

            // Keep identity and sequence history after timeout.
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
        Serial.println(static_cast<int>(objects[i].zone));

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
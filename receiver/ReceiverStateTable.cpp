
#include "ReceiverStateTable.h"

ReceiverStateTable::ReceiverStateTable()
{
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        objects[i].valid = false;
        objects[i].explicitly_cleared = false;
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
    const uint8_t difference =
        static_cast<uint8_t>(new_sequence - old_sequence);

    return difference != 0 && difference < 128;
}

bool ReceiverStateTable::isDuplicate(
    uint8_t vehicle_id,
    uint8_t object_id,
    uint8_t sequence_number) const
{
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].vehicle_id == vehicle_id &&
            objects[i].object_id == object_id &&
            objects[i].last_sequence == sequence_number &&
            objects[i].last_sequence != 0)
        {
            return true;
        }
    }

    return false;
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
    const unsigned long now = millis();

    // 1. Update an existing active object.
    NeighborObject *existing =
        findObject(message.vehicle_id, message.object_id);

    if (existing != nullptr)
    {
        if (message.sequence_number == existing->last_sequence)
        {
            Serial.println("Duplicate FULL received.");
            return false;
        }

        if (!isNewerSequence(
                message.sequence_number,
                existing->last_sequence))
        {
            Serial.println("Old FULL ignored.");
            return false;
        }

        existing->distance_cm = message.distance_cm;
        existing->zone = message.zone;
        existing->last_sequence = message.sequence_number;
        existing->last_updated = now;
        existing->explicitly_cleared = false;

        Serial.println("Existing object updated from FULL.");
        return true;
    }

    // 2. Restore an inactive object if its history is retained.
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].valid ||
            objects[i].vehicle_id != message.vehicle_id ||
            objects[i].object_id != message.object_id ||
            objects[i].last_sequence == 0)
        {
            continue;
        }

        if (message.sequence_number == objects[i].last_sequence)
        {
            Serial.println("Duplicate FULL received.");
            return false;
        }

        if (!isNewerSequence(
                message.sequence_number,
                objects[i].last_sequence))
        {
            Serial.println("Old FULL after CLEAR/timeout ignored.");
            return false;
        }

        objects[i].valid = true;
        objects[i].explicitly_cleared = false;
        objects[i].distance_cm = message.distance_cm;
        objects[i].zone = message.zone;
        objects[i].last_sequence = message.sequence_number;
        objects[i].last_updated = now;

        Serial.println("Object restored from newer FULL.");
        return true;
    }

    // 3. Prefer a never-used slot.
    int freeSlot = -1;

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (!objects[i].valid &&
            objects[i].last_sequence == 0)
        {
            freeSlot = i;
            break;
        }
    }

    // 4. Otherwise reuse the oldest inactive slot.
    // Never evict an active object.
    if (freeSlot == -1)
    {
        unsigned long oldestTime = 0xFFFFFFFFUL;

        for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
        {
            if (!objects[i].valid &&
                objects[i].last_updated <= oldestTime)
            {
                oldestTime = objects[i].last_updated;
                freeSlot = i;
            }
        }
    }

    // 5. Reject only if every slot is active.
    if (freeSlot == -1)
    {
        Serial.println("Receiver state table FULL: all slots active.");
        return false;
    }

    // 6. Initialize the selected slot.
    objects[freeSlot].valid = true;
    objects[freeSlot].explicitly_cleared = false;
    objects[freeSlot].vehicle_id = message.vehicle_id;
    objects[freeSlot].object_id = message.object_id;
    objects[freeSlot].distance_cm = message.distance_cm;
    objects[freeSlot].zone = message.zone;
    objects[freeSlot].last_sequence = message.sequence_number;
    objects[freeSlot].last_updated = now;

    Serial.println("New object added to receiver state table.");
    return true;
}

bool ReceiverStateTable::processDelta(
    const BlindLinkMessage &message)
{
    NeighborObject *object =
        findObject(message.vehicle_id, message.object_id);

    // 1. Apply DELTA to an active object.
    if (object != nullptr)
    {
        if (message.sequence_number == object->last_sequence)
        {
            Serial.println("Duplicate DELTA received.");
            return false;
        }

        if (!isNewerSequence(
                message.sequence_number,
                object->last_sequence))
        {
            Serial.println("Old DELTA ignored.");
            return false;
        }

        if (message.changed_fields & 0x01U)
        {
            object->distance_cm = message.distance_cm;
        }

        if (message.changed_fields & 0x02U)
        {
            object->zone = message.zone;
        }

        object->last_sequence = message.sequence_number;
        object->last_updated = millis();

        Serial.println("DELTA applied.");
        return true;
    }

    // 2. Check retained history for an inactive object.
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].vehicle_id != message.vehicle_id ||
            objects[i].object_id != message.object_id ||
            objects[i].last_sequence == 0)
        {
            continue;
        }

        if (message.sequence_number == objects[i].last_sequence)
        {
            Serial.println("Duplicate DELTA received.");
            return false;
        }

        if (!isNewerSequence(
                message.sequence_number,
                objects[i].last_sequence))
        {
            Serial.println("Old DELTA ignored.");
            return false;
        }

        // An explicitly cleared object must be reintroduced by FULL.
        if (objects[i].explicitly_cleared)
        {
            Serial.println(
                "DELTA rejected: object was explicitly cleared.");
            return false;
        }

        // Restore an object that disappeared due to timeout.
        if (message.changed_fields & 0x01U)
        {
            objects[i].distance_cm = message.distance_cm;
        }

        if (message.changed_fields & 0x02U)
        {
            objects[i].zone = message.zone;
        }

        objects[i].last_sequence = message.sequence_number;
        objects[i].last_updated = millis();
        objects[i].valid = true;
        objects[i].explicitly_cleared = false;

        Serial.println("Object restored from DELTA after timeout.");
        return true;
    }

    Serial.println("DELTA received for unknown object.");
    return false;
}

bool ReceiverStateTable::processClear(
    const BlindLinkMessage &message)
{
    // 1. Find an active object.
    NeighborObject *object =
        findObject(message.vehicle_id, message.object_id);

    if (object != nullptr)
    {
        if (message.sequence_number == object->last_sequence)
        {
            Serial.println("Duplicate CLEAR received.");
            return false;
        }

        if (!isNewerSequence(
                message.sequence_number,
                object->last_sequence))
        {
            Serial.println("Old CLEAR ignored.");
            return false;
        }

        object->last_sequence = message.sequence_number;
        object->last_updated = millis();
        object->valid = false;
        object->explicitly_cleared = true;

        Serial.println("Object cleared.");
        return true;
    }

    // 2. Find retained history for an inactive object.
    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].vehicle_id != message.vehicle_id ||
            objects[i].object_id != message.object_id ||
            objects[i].last_sequence == 0)
        {
            continue;
        }

        if (message.sequence_number == objects[i].last_sequence)
        {
            Serial.println("Duplicate CLEAR received.");
            return false;
        }

        if (!isNewerSequence(
                message.sequence_number,
                objects[i].last_sequence))
        {
            Serial.println("Old CLEAR ignored.");
            return false;
        }

        objects[i].last_sequence = message.sequence_number;
        objects[i].last_updated = millis();
        objects[i].valid = false;
        objects[i].explicitly_cleared = true;

        Serial.println("CLEAR accepted for inactive object.");
        return true;
    }

    // 3. No history exists. The object is already absent, so
    // accept this CLEAR idempotently to allow an ACK.
    // Do not create history here because the table may be full.
    Serial.println(
        "CLEAR for unknown object; accepted idempotently.");

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
        // Preserve the object's identity and sequence history.
        object->valid = false;
    }
}

void ReceiverStateTable::removeExpired(unsigned long timeout_ms)
{
    const unsigned long now = millis();

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        if (objects[i].valid &&
            now - objects[i].last_updated > timeout_ms)
        {
            Serial.print("Object timed out: ");
            Serial.println(objects[i].object_id);

            // A timeout is not an explicit CLEAR.
            objects[i].valid = false;
            objects[i].explicitly_cleared = false;
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

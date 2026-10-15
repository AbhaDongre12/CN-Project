#include "PriorityManager.h"

PriorityManager::PriorityManager()
{
}

NeighborObject *PriorityManager::getHighestPriorityObject(
    ReceiverStateTable &stateTable,
    WarningManager &warningManager)
{
    NeighborObject *highestPriorityObject = nullptr;

    WarningLevel highestLevel = WARNING_NONE;

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        NeighborObject *object = stateTable.getObjectAt(i);

        if (object == nullptr)
        {
            continue;
        }

        WarningLevel level =
            warningManager.evaluate(*object);

        if (level > highestLevel)
        {
            highestLevel = level;
            highestPriorityObject = object;
        }
    }

    return highestPriorityObject;
}

WarningLevel PriorityManager::getHighestPriorityLevel(
    ReceiverStateTable &stateTable,
    WarningManager &warningManager)
{
    WarningLevel highestLevel = WARNING_NONE;

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        NeighborObject *object = stateTable.getObjectAt(i);

        if (object == nullptr || !object->valid)
            continue;

        WarningLevel level = warningManager.evaluate(*object);

        Serial.printf(
            "Priority check: vehicle=%u object=%u distance=%u cm level=%d\n",
            object->vehicle_id,
            object->object_id,
            object->distance_cm,
            static_cast<int>(level));

        if (level > highestLevel)
            highestLevel = level;
    }

    Serial.printf(
        "Highest priority warning level=%d\n",
        static_cast<int>(highestLevel));

    return highestLevel;
}

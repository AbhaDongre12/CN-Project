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

        if (object == nullptr)
        {
            continue;
        }

        WarningLevel level =
            warningManager.evaluate(*object);

        if (level > highestLevel)
        {
            highestLevel = level;
        }
    }

    return highestLevel;
}
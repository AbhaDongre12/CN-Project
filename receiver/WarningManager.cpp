#include "WarningManager.h"

WarningManager::WarningManager()
{
    currentLevel = WARNING_NONE;
}

WarningLevel WarningManager::determineLevel(
    uint16_t distance_cm,
    Zone zone)
{
    /*
     * Prototype thresholds.
     *
     * These are NOT final safety thresholds.
     * They can be calibrated later using the actual
     * sensor and project requirements.
     */

    if (distance_cm <= 100)
    {
        return WARNING_DANGER;
    }

    if (distance_cm <= 200)
    {
        return WARNING_CAUTION;
    }

    return WARNING_NONE;
}

WarningLevel WarningManager::evaluate(
    const NeighborObject &object)
{
    if (!object.valid)
    {
        return WARNING_NONE;
    }

    return determineLevel(
        object.distance_cm,
        object.zone);
}

WarningLevel WarningManager::evaluateAll(
    ReceiverStateTable &stateTable)
{
    WarningLevel highestLevel = WARNING_NONE;

    for (int i = 0; i < MAX_NEIGHBOR_OBJECTS; i++)
    {
        NeighborObject *object =
            stateTable.getObjectAt(i);

        if (object == nullptr)
        {
            continue;
        }

        WarningLevel objectLevel =
            evaluate(*object);

        if (objectLevel > highestLevel)
        {
            highestLevel = objectLevel;
        }
    }

    currentLevel = highestLevel;

    return currentLevel;
}

void WarningManager::update(
    const NeighborObject &object)
{
    currentLevel = evaluate(object);

    printWarning(currentLevel);
}

void WarningManager::clearWarning()
{
    currentLevel = WARNING_NONE;

    Serial.println("WARNING CLEARED");
}

WarningLevel WarningManager::getCurrentLevel()
{
    return currentLevel;
}

void WarningManager::printWarning(
    WarningLevel level)
{
    switch (level)
    {
    case WARNING_NONE:
        Serial.println("WARNING: NONE");
        break;

    case WARNING_CAUTION:
        Serial.println("WARNING: CAUTION");
        break;

    case WARNING_DANGER:
        Serial.println("WARNING: DANGER");
        break;
    }
}
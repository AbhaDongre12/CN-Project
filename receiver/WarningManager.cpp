#include "WarningManager.h"

WarningManager::WarningManager()
{
    currentLevel = WARNING_NONE;
}

WarningLevel WarningManager::determineLevel(
    uint16_t distance_cm,
    Zone zone)
{
    // TESTING THRESHOLDS
    if (distance_cm <= 10)
    {
        return WARNING_DANGER;
    }

    if (distance_cm <= 20)
    {
        return WARNING_CAUTION;
    }

    if (distance_cm <= 30)
    {
        return WARNING_NONE;
    }

    return WARNING_NONE;
}

WarningLevel WarningManager::evaluate(
    const NeighborObject &object)
{
    if (!object.valid)
        return WARNING_NONE;

    WarningLevel level = determineLevel(
        object.distance_cm,
        object.zone);

    Serial.printf(
        "WARNING DEBUG: distance=%u cm, level=%d\n",
        object.distance_cm,
        static_cast<int>(level));

    return level;
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
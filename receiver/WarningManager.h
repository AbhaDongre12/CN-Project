#pragma once

#include <Arduino.h>
#include "ReceiverStateTable.h"

enum WarningLevel
{
    WARNING_NONE,
    WARNING_CAUTION,
    WARNING_DANGER
};

class WarningManager
{
public:
    WarningManager();

    WarningLevel evaluate(const NeighborObject &object);

    WarningLevel evaluateAll(
        ReceiverStateTable &stateTable);

    void update(const NeighborObject &object);

    void clearWarning();

    WarningLevel getCurrentLevel();

    void printWarning(WarningLevel level);

private:
    WarningLevel currentLevel;

    WarningLevel determineLevel(
        uint16_t distance_cm,
        Zone zone);
};
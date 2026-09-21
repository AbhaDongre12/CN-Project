#pragma once

#include <Arduino.h>
#include "ReceiverStateTable.h"
#include "WarningManager.h"

class PriorityManager
{
public:
    PriorityManager();

    NeighborObject *getHighestPriorityObject(
        ReceiverStateTable &stateTable,
        WarningManager &warningManager);

    WarningLevel getHighestPriorityLevel(
        ReceiverStateTable &stateTable,
        WarningManager &warningManager);
};
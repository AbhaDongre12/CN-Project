#pragma once

#include <Arduino.h>
#include "ReceiverProtocol.h"
#include "ReceiverStateTable.h"
#include "WarningManager.h"
#include "PriorityManager.h"

class ReceiverCommunication
{
public:
    ReceiverCommunication(
        ReceiverStateTable &stateTable,
        WarningManager &warningManager,
        PriorityManager &priorityManager);

    bool begin();

    void processMessage(
        const BlindLinkMessage &message);

private:
    ReceiverStateTable &stateTable;
    WarningManager &warningManager;
    PriorityManager &priorityManager;

    void handleMessage(
        const BlindLinkMessage &message);
};
#pragma once

#include <Arduino.h>
#include "ReceiverProtocol.h"
#include "ReceiverStateTable.h"
#include "WarningManager.h"

class ReceiverCommunication
{
public:
    ReceiverCommunication(
        ReceiverStateTable &stateTable,
        WarningManager &warningManager);

    bool begin();

    void processMessage(
        const BlindLinkMessage &message);

private:
    ReceiverStateTable &stateTable;
    WarningManager &warningManager;

    void handleMessage(
        const BlindLinkMessage &message);
};
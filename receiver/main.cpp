
#include <Arduino.h>

#include "ReceiverStateTable.h"
#include "WarningManager.h"
#include "ReceiverCommunication.h"
#include "PriorityManager.h"

ReceiverStateTable stateTable;
WarningManager warningManager;
PriorityManager priorityManager;

ReceiverCommunication receiver(
    stateTable,
    warningManager,
    priorityManager);

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("======================================");
    Serial.println("       BLINDLINK LIVE RECEIVER");
    Serial.println("======================================");

    if (!receiver.begin())
    {
        Serial.println("Receiver initialization failed!");
        return;
    }

    Serial.println("Receiver initialized. Waiting for packets...");
}

void loop()
{
    receiver.poll();
    delay(5);
}

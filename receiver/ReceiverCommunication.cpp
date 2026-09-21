#include "ReceiverCommunication.h"

ReceiverCommunication::ReceiverCommunication(
    ReceiverStateTable &stateTable,
    WarningManager &warningManager,
    PriorityManager &priorityManager)
    : stateTable(stateTable),
      warningManager(warningManager),
      priorityManager(priorityManager)
{
}

bool ReceiverCommunication::begin()
{
    Serial.println("Receiver communication initialized.");
    return true;
}

void ReceiverCommunication::processMessage(
    const BlindLinkMessage &message)
{
    handleMessage(message);
}

void ReceiverCommunication::handleMessage(
    const BlindLinkMessage &message)
{
    bool accepted = stateTable.processMessage(message);

    if (!accepted)
    {
        return;
    }

    if (message.type == MESSAGE_CLEAR)
    {
        warningManager.clearWarning();
        return;
    }

    NeighborObject *object =
        stateTable.findObject(
            message.vehicle_id,
            message.object_id);

    if (object != nullptr)
    {
        warningManager.update(*object);

        NeighborObject *priorityObject =
            priorityManager.getHighestPriorityObject(
                stateTable,
                warningManager);

        if (priorityObject != nullptr)
        {
            Serial.print("Priority object: ");
            Serial.println(
                static_cast<int>(priorityObject->object_id));

            Serial.print("Priority warning: ");
            priorityManager.getHighestPriorityLevel(
                stateTable,
                warningManager);
        }
    }
}
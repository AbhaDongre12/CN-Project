#include "ReceiverCommunication.h"

ReceiverCommunication::ReceiverCommunication(
    ReceiverStateTable &stateTable,
    WarningManager &warningManager)
    : stateTable(stateTable),
      warningManager(warningManager)
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
    // Process the incoming message
    bool processed = stateTable.processMessage(message);

    if (!processed)
    {
        Serial.println("Message processing failed.");
        return;
    }

    // CLEAR should remove the corresponding warning
    if (message.type == MESSAGE_CLEAR)
    {
        warningManager.clearWarning();
        return;
    }

    // Find the current object after state update
    NeighborObject *object =
        stateTable.findObject(
            message.vehicle_id,
            message.object_id);

    if (object == nullptr)
    {
        return;
    }

    // Evaluate the updated state
    warningManager.update(*object);
}
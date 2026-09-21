#include <iostream>

#include "../receiver/ReceiverProtocol.h"
#include "../receiver/ReceiverStateTable.h"
#include "../receiver/WarningManager.h"
#include "../receiver/PriorityManager.h"
#include "../receiver/ReceiverCommunication.h"

void printWarningLevel(WarningLevel level)
{
    switch (level)
    {
    case WARNING_NONE:
        std::cout << "NONE";
        break;

    case WARNING_CAUTION:
        std::cout << "CAUTION";
        break;

    case WARNING_DANGER:
        std::cout << "DANGER";
        break;
    }
}

int main()
{
    ReceiverStateTable table;
    WarningManager warningManager;
    PriorityManager priorityManager;
    std::cout << "========================================\n";
    std::cout << "   BLINDLINK RECEIVER NATIVE TEST\n";
    std::cout << "========================================\n\n";

    // ==================================================
    // TEST 1
    // FULL MESSAGE
    // ==================================================

    BlindLinkMessage full{};
    full.type = MESSAGE_FULL;
    full.vehicle_id = 1;
    full.object_id = 1;
    full.changed_fields = 0;
    full.distance_cm = 250;
    full.zone = ZONE_LEFT;
    full.sequence_number = 1;

    std::cout << "[TEST 1] FULL message - 250 cm\n";

    bool result = table.processMessage(full);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Warning: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    // ==================================================
    // TEST 2
    // DISTANCE DELTA
    // ==================================================

    BlindLinkMessage deltaDistance{};
    deltaDistance.type = MESSAGE_DELTA;
    deltaDistance.vehicle_id = 1;
    deltaDistance.object_id = 1;
    deltaDistance.changed_fields = 0x01;
    deltaDistance.distance_cm = 180;
    deltaDistance.zone = ZONE_LEFT;
    deltaDistance.sequence_number = 2;

    std::cout << "[TEST 2] DELTA distance - 180 cm\n";

    result = table.processMessage(deltaDistance);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Warning: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    // ==================================================
    // TEST 3
    // ZONE DELTA
    // ==================================================

    BlindLinkMessage deltaZone{};
    deltaZone.type = MESSAGE_DELTA;
    deltaZone.vehicle_id = 1;
    deltaZone.object_id = 1;
    deltaZone.changed_fields = 0x02;
    deltaZone.distance_cm = 180;
    deltaZone.zone = ZONE_RIGHT;
    deltaZone.sequence_number = 3;

    std::cout << "[TEST 3] DELTA zone - RIGHT\n";

    result = table.processMessage(deltaZone);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Warning: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    // ==================================================
    // TEST 4
    // OLD SEQUENCE
    // ==================================================

    BlindLinkMessage oldMessage{};
    oldMessage.type = MESSAGE_DELTA;
    oldMessage.vehicle_id = 1;
    oldMessage.object_id = 1;
    oldMessage.changed_fields = 0x01;
    oldMessage.distance_cm = 999;
    oldMessage.zone = ZONE_RIGHT;
    oldMessage.sequence_number = 2;

    std::cout << "[TEST 4] OLD sequence - should be rejected\n";

    result = table.processMessage(oldMessage);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Warning after rejected packet: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    // ==================================================
    // TEST 5
    // ADD CAUTION OBJECT
    // ==================================================

    BlindLinkMessage object2{};
    object2.type = MESSAGE_FULL;
    object2.vehicle_id = 1;
    object2.object_id = 2;
    object2.changed_fields = 0;
    object2.distance_cm = 180;
    object2.zone = ZONE_LEFT;
    object2.sequence_number = 1;

    std::cout << "[TEST 5] Add second object - 180 cm\n";

    result = table.processMessage(object2);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Overall warning: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    // ==================================================
    // TEST 6
    // ADD DANGER OBJECT
    // ==================================================

    BlindLinkMessage object3{};
    object3.type = MESSAGE_FULL;
    object3.vehicle_id = 2;
    object3.object_id = 1;
    object3.changed_fields = 0;
    object3.distance_cm = 80;
    object3.zone = ZONE_REAR;
    object3.sequence_number = 1;

    std::cout << "[TEST 6] Add third object - 80 cm\n";

    result = table.processMessage(object3);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Overall warning: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    // ==================================================
    // TEST 7
    // CLEAR DANGER OBJECT
    // ==================================================

    BlindLinkMessage clear{};
    clear.type = MESSAGE_CLEAR;
    clear.vehicle_id = 2;
    clear.object_id = 1;
    clear.changed_fields = 0;
    clear.distance_cm = 0;
    clear.zone = ZONE_REAR;
    clear.sequence_number = 2;

    std::cout << "[TEST 7] Clear dangerous object\n";

    result = table.processMessage(clear);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    std::cout << "Overall warning after CLEAR: ";
    printWarningLevel(warningManager.evaluateAll(table));
    std::cout << "\n\n";

    std::cout << "[TEST 8] Priority Manager\n";

    NeighborObject *priorityObject =
        priorityManager.getHighestPriorityObject(
            table,
            warningManager);

    if (priorityObject != nullptr)
    {
        std::cout << "Highest priority object: "
                  << static_cast<int>(priorityObject->object_id)
                  << "\n";

        std::cout << "Vehicle: "
                  << static_cast<int>(priorityObject->vehicle_id)
                  << "\n";

        std::cout << "Distance: "
                  << priorityObject->distance_cm
                  << " cm\n";

        std::cout << "Warning: ";

        printWarningLevel(
            warningManager.evaluate(*priorityObject));

        std::cout << "\n";
    }
    else
    {
        std::cout << "No active priority object.\n";
    }

    std::cout << "\n";

    std::cout << "[TEST 9] DANGER object gets highest priority\n";

    BlindLinkMessage danger{};
    danger.type = MESSAGE_FULL;
    danger.vehicle_id = 2;
    danger.object_id = 3;
    danger.changed_fields = 0;
    danger.distance_cm = 80;
    danger.zone = ZONE_REAR;
    danger.sequence_number = 1;

    result = table.processMessage(danger);

    std::cout << "Danger object added: "
              << (result ? "YES" : "NO")
              << "\n";

    priorityObject =
        priorityManager.getHighestPriorityObject(
            table,
            warningManager);

    if (priorityObject != nullptr)
    {
        std::cout << "Priority object: "
                  << static_cast<int>(priorityObject->object_id)
                  << "\n";

        std::cout << "Priority warning: ";

        printWarningLevel(
            warningManager.evaluate(*priorityObject));

        std::cout << "\n";
    }

    std::cout << "\n";

    std::cout << "[TEST 10] DELTA for unknown object\n";

    BlindLinkMessage unknownDelta{};
    unknownDelta.type = MESSAGE_DELTA;
    unknownDelta.vehicle_id = 9;
    unknownDelta.object_id = 9;
    unknownDelta.changed_fields = 0x01;
    unknownDelta.distance_cm = 100;
    unknownDelta.zone = ZONE_LEFT;
    unknownDelta.sequence_number = 1;

    result = table.processMessage(unknownDelta);

    std::cout << "Accepted: "
              << (result ? "YES" : "NO")
              << "\n\n";

    std::cout << "[TEST 11] Sequence number wraparound\n";

    BlindLinkMessage wrap1{};
    wrap1.type = MESSAGE_FULL;
    wrap1.vehicle_id = 3;
    wrap1.object_id = 1;
    wrap1.distance_cm = 200;
    wrap1.zone = ZONE_LEFT;
    wrap1.sequence_number = 255;

    result = table.processMessage(wrap1);

    std::cout << "Sequence 255 accepted: "
              << (result ? "YES" : "NO")
              << "\n";

    BlindLinkMessage wrap2{};
    wrap2.type = MESSAGE_DELTA;
    wrap2.vehicle_id = 3;
    wrap2.object_id = 1;
    wrap2.changed_fields = 0x01;
    wrap2.distance_cm = 150;
    wrap2.zone = ZONE_LEFT;
    wrap2.sequence_number = 0;

    result = table.processMessage(wrap2);

    std::cout << "Sequence 0 after 255 accepted: "
              << (result ? "YES" : "NO")
              << "\n\n";

    std::cout << "[TEST 12] Empty receiver state\n";

    ReceiverStateTable emptyTable;
    WarningManager emptyWarningManager;
    PriorityManager emptyPriorityManager;

    NeighborObject *emptyPriority =
        emptyPriorityManager.getHighestPriorityObject(
            emptyTable,
            emptyWarningManager);

    std::cout << "Priority object: "
              << (emptyPriority == nullptr ? "NONE" : "FOUND")
              << "\n";

    std::cout << "Warning: ";

    printWarningLevel(
        emptyPriorityManager.getHighestPriorityLevel(
            emptyTable,
            emptyWarningManager));

    std::cout << "\n\n";

    std::cout << "[TEST 13] Receiver Communication Integration\n";

    ReceiverStateTable integrationTable;
    WarningManager integrationWarning;
    PriorityManager integrationPriority;

    ReceiverCommunication communication(
        integrationTable,
        integrationWarning,
        integrationPriority);

    communication.begin();

    BlindLinkMessage integrationMessage{};
    integrationMessage.type = MESSAGE_FULL;
    integrationMessage.vehicle_id = 5;
    integrationMessage.object_id = 1;
    integrationMessage.changed_fields = 0;
    integrationMessage.distance_cm = 80;
    integrationMessage.zone = ZONE_LEFT;
    integrationMessage.sequence_number = 1;

    communication.processMessage(integrationMessage);

    NeighborObject *integrationObject =
        integrationTable.findObject(5, 1);

    std::cout << "Object received: "
              << (integrationObject != nullptr ? "YES" : "NO")
              << "\n";

    std::cout << "Warning: ";

    printWarningLevel(
        integrationWarning.evaluateAll(integrationTable));

    std::cout << "\n";

    NeighborObject *integrationPriorityObject =
        integrationPriority.getHighestPriorityObject(
            integrationTable,
            integrationWarning);

    std::cout << "Priority object: ";

    if (integrationPriorityObject != nullptr)
    {
        std::cout
            << static_cast<int>(
                   integrationPriorityObject->object_id);
    }
    else
    {
        std::cout << "NONE";
    }

    std::cout << "\n\n";
    // ==================================================
    // FINAL STATE
    // ==================================================

    std::cout << "========================================\n";
    std::cout << "FINAL RECEIVER STATE\n";
    std::cout << "========================================\n";

    table.printTable();

    std::cout << "\nActive objects: "
              << table.getActiveObjectCount()
              << "\n";

    std::cout << "\n========================================\n";
    std::cout << "           TEST COMPLETE\n";
    std::cout << "========================================\n";

    return 0;
}
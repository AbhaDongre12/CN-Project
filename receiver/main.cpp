#include <Arduino.h>

#include "ReceiverStateTable.h"
#include "WarningManager.h"
#include "ReceiverCommunication.h"

ReceiverStateTable stateTable;
WarningManager warningManager;

ReceiverCommunication receiver(
    stateTable,
    warningManager);

void showObject(uint8_t vehicleID, uint8_t objectID)
{
    NeighborObject *object =
        stateTable.findObject(vehicleID, objectID);

    if (object == nullptr)
    {
        Serial.println("  Object not found.");
        return;
    }

    Serial.print("  Vehicle ");
    Serial.print(object->vehicle_id);

    Serial.print(" | Object ");
    Serial.print(object->object_id);

    Serial.print(" | Distance ");
    Serial.print(object->distance_cm);
    Serial.print(" cm");

    Serial.print(" | Zone ");
    Serial.print((int)object->zone);

    Serial.print(" | Seq ");
    Serial.println(object->last_sequence);
}

void showWarning()
{
    WarningLevel level =
        warningManager.evaluateAll(stateTable);

    Serial.print("  Overall warning: ");

    switch (level)
    {
    case WARNING_NONE:
        Serial.println("NONE");
        break;

    case WARNING_CAUTION:
        Serial.println("CAUTION");
        break;

    case WARNING_DANGER:
        Serial.println("DANGER");
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("======================================");
    Serial.println("       BLINDLINK RECEIVER TEST");
    Serial.println("======================================");

    receiver.begin();

    // ==================================================
    // TEST 1
    // NEW OBJECT
    // ==================================================

    Serial.println();
    Serial.println("[TEST 1] Vehicle 1 -> Object 1 -> FULL");

    BlindLinkMessage msg1{};

    msg1.type = MESSAGE_FULL;
    msg1.vehicle_id = 1;
    msg1.object_id = 1;
    msg1.changed_fields = 0;
    msg1.distance_cm = 250;
    msg1.zone = ZONE_LEFT;
    msg1.sequence_number = 1;

    receiver.processMessage(msg1);

    showObject(1, 1);
    showWarning();

    // ==================================================
    // TEST 2
    // DISTANCE DELTA
    // ==================================================

    Serial.println();
    Serial.println("[TEST 2] Object 1 distance changes");

    BlindLinkMessage msg2{};

    msg2.type = MESSAGE_DELTA;
    msg2.vehicle_id = 1;
    msg2.object_id = 1;
    msg2.changed_fields = 0x01;
    msg2.distance_cm = 180;
    msg2.zone = ZONE_LEFT;
    msg2.sequence_number = 2;

    receiver.processMessage(msg2);

    showObject(1, 1);
    showWarning();

    // ==================================================
    // TEST 3
    // ZONE DELTA
    // ==================================================

    Serial.println();
    Serial.println("[TEST 3] Object 1 changes zone");

    BlindLinkMessage msg3{};

    msg3.type = MESSAGE_DELTA;
    msg3.vehicle_id = 1;
    msg3.object_id = 1;
    msg3.changed_fields = 0x02;
    msg3.distance_cm = 180;
    msg3.zone = ZONE_RIGHT;
    msg3.sequence_number = 3;

    receiver.processMessage(msg3);

    showObject(1, 1);
    showWarning();

    // ==================================================
    // TEST 4
    // OLD PACKET
    // ==================================================

    Serial.println();
    Serial.println("[TEST 4] OLD packet should be rejected");

    BlindLinkMessage msg4{};

    msg4.type = MESSAGE_DELTA;
    msg4.vehicle_id = 1;
    msg4.object_id = 1;
    msg4.changed_fields = 0x01;
    msg4.distance_cm = 999;
    msg4.zone = ZONE_RIGHT;

    // Current sequence is 3.
    // This is older.
    msg4.sequence_number = 2;

    receiver.processMessage(msg4);

    showObject(1, 1);
    showWarning();

    // ==================================================
    // TEST 5
    // SECOND OBJECT
    // ==================================================

    Serial.println();
    Serial.println("[TEST 5] Add second object");

    BlindLinkMessage msg5{};

    msg5.type = MESSAGE_FULL;
    msg5.vehicle_id = 1;
    msg5.object_id = 2;
    msg5.changed_fields = 0;
    msg5.distance_cm = 80;
    msg5.zone = ZONE_LEFT;
    msg5.sequence_number = 1;

    receiver.processMessage(msg5);

    showObject(1, 1);
    showObject(1, 2);
    showWarning();

    // ==================================================
    // TEST 6
    // SECOND VEHICLE
    // ==================================================

    Serial.println();
    Serial.println("[TEST 6] Add object from Vehicle 2");

    BlindLinkMessage msg6{};

    msg6.type = MESSAGE_FULL;
    msg6.vehicle_id = 2;
    msg6.object_id = 1;
    msg6.changed_fields = 0;
    msg6.distance_cm = 300;
    msg6.zone = ZONE_RIGHT;
    msg6.sequence_number = 1;

    receiver.processMessage(msg6);

    showObject(1, 1);
    showObject(1, 2);
    showObject(2, 1);
    showWarning();

    // ==================================================
    // TEST 7
    // CLEAR ONE OBJECT
    // ==================================================

    Serial.println();
    Serial.println("[TEST 7] Clear dangerous Object 2");

    BlindLinkMessage msg7{};

    msg7.type = MESSAGE_CLEAR;
    msg7.vehicle_id = 1;
    msg7.object_id = 2;
    msg7.changed_fields = 0;
    msg7.distance_cm = 0;
    msg7.zone = ZONE_LEFT;
    msg7.sequence_number = 2;

    receiver.processMessage(msg7);

    showObject(1, 2);
    showObject(2, 1);
    showWarning();

    // ==================================================
    // FINAL TABLE
    // ==================================================

    Serial.println();
    Serial.println("======================================");
    Serial.println("FINAL RECEIVER STATE TABLE");
    Serial.println("======================================");

    stateTable.printTable();

    Serial.println();
    Serial.println("======================================");
    Serial.println("           TEST COMPLETE");
    Serial.println("======================================");
}

void loop()
{
}
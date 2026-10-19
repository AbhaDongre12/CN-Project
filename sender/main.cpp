
#include <Arduino.h>
#include "CommunicationManager.h"
#include "ChangeDetector.h"
#include "StateTable.h"
#include "../common/sensor_manager.h"
#include "../common/object_tracker.h"
#include "../common/sensor_health_monitor.h"

constexpr unsigned long SENSOR_SAMPLE_GAP_MS = 60;
constexpr unsigned long STATE_INTERVAL_MS = 200;

CommunicationManager communicationManager;
StateTable stateTable;
ChangeDetector changeDetector(10);
MessageComposer messageComposer(1);
SensorManager sensorManager;
ObjectTracker tracker;
SensorHealthMonitor sensorHealth[SENDER_SENSOR_COUNT];

unsigned long lastSampleMs = 0;
unsigned long lastStateMs = 0;

struct PendingClear
{
    bool active = false;
    uint8_t object_id = 0;
    BlindLinkMessage message = {};
};

PendingClear pendingClears[MAX_OBJECTS];

const char *zoneName(Zone zone)
{
    switch (zone)
    {
    case ZONE_LEFT:
        return "LEFT";
    case ZONE_RIGHT:
        return "RIGHT";
    case ZONE_REAR:
        return "REAR";
    case ZONE_FRONT:
        return "FRONT";
    case ZONE_FRONT_LEFT:
        return "FRONT_LEFT";
    case ZONE_FRONT_RIGHT:
        return "FRONT_RIGHT";
    case ZONE_REAR_LEFT:
        return "REAR_LEFT";
    case ZONE_REAR_RIGHT:
        return "REAR_RIGHT";
    default:
        return "UNKNOWN";
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("======================================");
    Serial.println("  BLINDLINK 4-SENSOR LIVE SENDER");
    Serial.println("======================================");

    Serial.printf("Config array count: %u\n",
                  (unsigned)SENDER_SENSOR_COUNT);

    Serial.printf("Config array size: %u\n",
                  (unsigned)(sizeof(SENDER_SENSOR_CONFIG) /
                             sizeof(SENDER_SENSOR_CONFIG[0])));

    if (!sensorManager.begin(
            SENDER_SENSOR_CONFIG,
            SENDER_SENSOR_COUNT))
    {
        Serial.println("ERROR: Sensor manager configuration failed!");
        return;
    }

    stateTable.initialize();

    if (!communicationManager.begin(1))
    {
        Serial.println("ERROR: ESP-NOW initialization failed!");
        return;
    }

    Serial.println("ESP-NOW initialized successfully.");

    communicationManager.displayMacAddress();

    Serial.printf(
        "Configured sensors: %u\n",
        static_cast<unsigned>(sensorManager.count()));

    Serial.println("Sampling sensors sequentially...");
    Serial.println("Directions: FRONT_LEFT, FRONT_RIGHT, REAR_LEFT, REAR_RIGHT");
    Serial.println("--------------------------------------");
}

void loop()
{
    const unsigned long now = millis();

    // Read one sensor per interval to reduce acoustic crosstalk.
    if (now - lastSampleMs >= SENSOR_SAMPLE_GAP_MS)
    {
        lastSampleMs = now;

        const size_t index = sensorManager.nextIndex();

        SensorReading reading = {};
        const bool valid = sensorManager.read(index, &reading);

        sensorHealth[index].reportReading(valid);

        Serial.printf(
            "Sensor %u | Zone: %s | ",
            static_cast<unsigned>(index + 1),
            zoneName(reading.zone));

        if (valid)
        {
            Serial.printf(
                "Distance: %u cm | %s\n",
                reading.distance_cm,
                reading.object_present
                    ? "Object detected"
                    : "No object in detection range");
        }
        else
        {
            Serial.println(
                "Invalid echo; tracker state retained temporarily");
        }

        if (sensorHealth[index].faultSuspected())
        {
            Serial.printf(
                "WARNING: Repeated timeouts on sensor %u. "
                "Check sensor readings and configuration.\n",
                static_cast<unsigned>(index + 1));
        }

        tracker.update(reading, now);
    }

    // Process tracked-object state at the configured interval.
    if (now - lastStateMs < STATE_INTERVAL_MS)
    {
        return;
    }

    lastStateMs = now;

    TrackedObject active[MAX_OBJECTS];
    uint8_t expired[MAX_OBJECTS];

    const uint8_t activeCount =
        tracker.getActiveObjects(active);

    const uint8_t expiredCount =
        tracker.getExpiredObjects(expired);

    // Handle newly detected or changed objects.
    for (uint8_t i = 0; i < activeCount; ++i)
    {
        const TrackedObject &object = active[i];

        ChangeResult result =
            changeDetector.detectChange(object, stateTable);

        if (result.type != NEW_OBJECT &&
            result.type != CHANGED_OBJECT)
        {

            if (stateTable.findObject(object.object_id) >= 0)
            {
                stateTable.updateObject(object);
            }

            continue;
        }

        BlindLinkMessage message =
            messageComposer.compose(result);

        Serial.println();
        Serial.println("----- OBJECT UPDATE -----");

        Serial.printf(
            "Object ID: %u\n",
            message.object_id);

        Serial.printf(
            "Distance: %u cm\n",
            message.distance_cm);

        Serial.printf(
            "Zone: %s\n",
            zoneName(message.zone));

        Serial.printf(
            "Sequence: %u\n",
            message.sequence_number);

        Serial.println("Sending message to receiver...");

        if (communicationManager.sendReliableMessage(
                message, 3, 300))
        {

            Serial.println(
                "SUCCESS: Message acknowledged by receiver.");

            stateTable.addObject(object);
            stateTable.updateObject(object);
            stateTable.updateConfirmedState(
                object.object_id, object);
        }
        else
        {
            Serial.println(
                "WARNING: Delivery unconfirmed. "
                "Confirmed state unchanged.");

            if (stateTable.findObject(object.object_id) >= 0)
            {
                stateTable.updateObject(object);
            }
        }

        Serial.println("-------------------------");
    }

    // Queue CLEAR messages for expired objects.
    for (uint8_t i = 0; i < expiredCount; ++i)
    {
        const uint8_t expiredId = expired[i];

        bool alreadyQueued = false;

        for (uint8_t j = 0; j < MAX_OBJECTS; ++j)
        {
            if (pendingClears[j].active &&
                pendingClears[j].object_id == expiredId)
            {
                alreadyQueued = true;
                break;
            }
        }

        if (alreadyQueued)
        {
            tracker.confirmExpiredObjectQueued(expiredId);
            continue;
        }

        int freeSlot = -1;

        for (uint8_t j = 0; j < MAX_OBJECTS; ++j)
        {
            if (!pendingClears[j].active)
            {
                freeSlot = j;
                break;
            }
        }

        if (freeSlot < 0)
        {
            Serial.printf(
                "WARNING: No pending CLEAR slot for object %u. "
                "Tracker will retain it and retry.\n",
                expiredId);
            continue;
        }

        PendingClear &pending = pendingClears[freeSlot];

        pending.object_id = expiredId;
        pending.message = messageComposer.composeClear(expiredId);
        pending.active = true;

        if (!tracker.confirmExpiredObjectQueued(expiredId))
        {
            Serial.printf(
                "WARNING: Could not confirm CLEAR handoff "
                "for object %u.\n",
                expiredId);
        }
    }

    // Send pending CLEAR messages; retain them for retry on failure.
    for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
    {
        if (!pendingClears[i].active)
        {
            continue;
        }

        const BlindLinkMessage clearMessage =
            pendingClears[i].message;

        Serial.println();
        Serial.println("----- CLEAR UPDATE -----");

        Serial.printf(
            "Object ID: %u\n",
            clearMessage.object_id);

        Serial.printf(
            "Sequence: %u\n",
            clearMessage.sequence_number);

        Serial.println("Sending CLEAR message...");

        if (communicationManager.sendReliableMessage(
                clearMessage, 3, 300))
        {

            Serial.println(
                "SUCCESS: CLEAR acknowledged by receiver.");

            stateTable.removeObject(
                pendingClears[i].object_id);

            pendingClears[i].active = false;
        }
        else
        {
            Serial.println(
                "WARNING: CLEAR delivery unconfirmed; "
                "will retry.");
        }

        Serial.println("------------------------");
    }
}

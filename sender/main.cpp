
#include <Arduino.h>
#include "CommunicationManager.h"
#include "ChangeDetector.h"
#include "StateTable.h"
#include "../common/sensor_interface.h"
#include "../common/object_tracker.h"
#include "../common/sensor_health_monitor.h"

// HC-SR04 wiring:
// TRIG -> GPIO5 (D5)
// ECHO -> GPIO18 (D18), through a resistor voltage divider.
constexpr uint8_t HC_SR04_TRIG_PIN = 5;
constexpr uint8_t HC_SR04_ECHO_PIN = 18;

constexpr Zone THIS_SENSOR_ZONE = ZONE_LEFT;
constexpr uint16_t DETECTION_MAX_CM = 250;

constexpr unsigned long SAMPLE_INTERVAL_MS = 70;
constexpr unsigned long STATE_INTERVAL_MS = 200;
constexpr unsigned long FULL_REFRESH_INTERVAL_MS = 2000;

CommunicationManager communicationManager;
StateTable stateTable;
ChangeDetector changeDetector(10);
MessageComposer messageComposer(1);

SensorInterface sensor(
    HC_SR04_TRIG_PIN,
    HC_SR04_ECHO_PIN,
    THIS_SENSOR_ZONE,
    DETECTION_MAX_CM);

ObjectTracker tracker;
SensorHealthMonitor sensorHealth;

unsigned long lastSampleMs = 0;
unsigned long lastStateMs = 0;
unsigned long lastFullRefreshMs = 0;

struct PendingClear
{
    bool active = false;
    uint8_t object_id = 0;
    BlindLinkMessage message = {};
};

PendingClear pendingClears[MAX_OBJECTS];

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("======================================");
    Serial.println("     BLINDLINK LIVE SENSOR SENDER");
    Serial.println("======================================");

    sensor.begin();
    stateTable.initialize();

    if (!communicationManager.begin(1))
    {
        Serial.println("ESP-NOW initialization failed!");
        return;
    }

    Serial.println("ESP-NOW initialized successfully.");
    communicationManager.displayMacAddress();
    Serial.println("Reading live HC-SR04 distances...");
}

void loop()
{
    const unsigned long now = millis();

    // 1. Sample the ultrasonic sensor.
    if (now - lastSampleMs >= SAMPLE_INTERVAL_MS)
    {
        lastSampleMs = now;

        SensorReading reading = {};
        const bool valid = sensor.read(&reading);

        sensorHealth.reportReading(valid);

        if (valid)
        {
            Serial.print("HC-SR04 distance: ");
            Serial.print(reading.distance_cm);
            Serial.println(" cm");
        }
        else
        {
            Serial.println(
                "HC-SR04: no valid echo; retaining tracker state temporarily.");
        }

        tracker.update(reading, now);

        if (sensorHealth.faultSuspected())
        {
            Serial.println(
                "WARNING: repeated HC-SR04 timeouts; check wiring or target.");
        }
    }

    // 2. Process tracked objects.
    if (now - lastStateMs < STATE_INTERVAL_MS)
        return;

    lastStateMs = now;

    TrackedObject active[MAX_OBJECTS];
    uint8_t expired[MAX_OBJECTS];

    const uint8_t activeCount = tracker.getActiveObjects(active);
    const uint8_t expiredCount = tracker.getExpiredObjects(expired);

    // 3. Send new, changed, or periodically refreshed object information.
    for (uint8_t i = 0; i < activeCount; ++i)
    {
        const TrackedObject &object = active[i];

        ChangeResult result =
            changeDetector.detectChange(object, stateTable);

        Serial.printf(
            "Tracked object id=%u distance=%u cm zone=%u\n",
            object.object_id,
            object.distance_cm,
            static_cast<uint8_t>(object.zone));

        const bool refreshDue =
            now - lastFullRefreshMs >= FULL_REFRESH_INTERVAL_MS;

        if (result.type == NEW_OBJECT ||
            result.type == CHANGED_OBJECT ||
            refreshDue)
        {
            BlindLinkMessage message = {};

            if (result.type == NEW_OBJECT || refreshDue)
            {
                message.vehicle_id = 1;
                message.object_id = object.object_id;
                message.type = MESSAGE_FULL;
                message.changed_fields = FIELD_DISTANCE | FIELD_ZONE;
                message.distance_cm = object.distance_cm;
                message.zone = object.zone;
                message.sequence_number =
                    messageComposer.getNextSequenceNumber();

                Serial.printf(
                    "Sending FULL object=%u distance=%u seq=%u\n",
                    message.object_id,
                    message.distance_cm,
                    message.sequence_number);
            }
            else
            {
                message = messageComposer.compose(result);

                Serial.printf(
                    "Sending DELTA object=%u distance=%u seq=%u\n",
                    message.object_id,
                    message.distance_cm,
                    message.sequence_number);
            }

            const bool delivered =
                communicationManager.sendReliableMessage(
                    message, 3, 300);

            if (delivered)
            {
                Serial.println("Message acknowledged by receiver.");

                stateTable.updateObject(object);
                stateTable.updateConfirmedState(object.object_id, object);

                if (message.type == MESSAGE_FULL)
                    lastFullRefreshMs = now;
            }
            else
            {
                Serial.println(
                    "Delivery unconfirmed; message will be retried "
                    "when the next state update requires it.");
            }
        }
        else
        {
            // Keep the latest local reading without changing
            // the last confirmed state.
            if (stateTable.findObject(object.object_id) >= 0)
                stateTable.updateObject(object);
        }
    }

    // 4. Queue CLEAR messages for expired objects.
    for (uint8_t i = 0; i < expiredCount; ++i)
    {
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
                "No pending CLEAR slot for expired object %u.\n",
                expired[i]);
            continue;
        }

        pendingClears[freeSlot].active = true;
        pendingClears[freeSlot].object_id = expired[i];

        pendingClears[freeSlot].message =
            messageComposer.composeClear(expired[i]);
    }

    // 5. Retry pending CLEAR messages until acknowledged.
    for (uint8_t i = 0; i < MAX_OBJECTS; ++i)
    {
        if (!pendingClears[i].active)
            continue;

        const BlindLinkMessage clearMessage =
            pendingClears[i].message;

        Serial.printf(
            "Sending CLEAR object=%u seq=%u\n",
            clearMessage.object_id,
            clearMessage.sequence_number);

        if (communicationManager.sendReliableMessage(
                clearMessage, 3, 300))
        {
            Serial.println("CLEAR acknowledged by receiver.");

            stateTable.removeObject(
                pendingClears[i].object_id);

            pendingClears[i].active = false;
        }
        else
        {
            Serial.println(
                "CLEAR delivery unconfirmed; will retry.");
        }
    }
}

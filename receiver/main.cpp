#include <Arduino.h>
#include "ReceiverStateTable.h"
#include "WarningManager.h"
#include "ReceiverCommunication.h"
#include "PriorityManager.h"
#include "../common/sensor_manager.h"
#include "../common/sensor_health_monitor.h"

// Hardware and timing settings.
constexpr uint8_t BUZZER_PIN = 25;
constexpr unsigned long SENSOR_SAMPLE_GAP_MS = 70;
constexpr unsigned long LOCAL_SENSOR_STALE_MS = 1000;
constexpr unsigned long CAUTION_ON_MS = 180;
constexpr unsigned long CAUTION_OFF_MS = 650;

// Prototype local warning thresholds. Keep these aligned with WarningManager.
constexpr uint16_t DANGER_DISTANCE_CM = 100;
constexpr uint16_t CAUTION_DISTANCE_CM = 200;

ReceiverStateTable stateTable;
WarningManager warningManager;
PriorityManager priorityManager;
ReceiverCommunication receiver(stateTable, warningManager, priorityManager);
SensorManager localSensors;
SensorHealthMonitor localSensorHealth[MAX_CONFIGURED_SENSORS];

struct LocalSensorState {
    bool has_recent_detection = false;
    uint16_t distance_cm = 0;
    unsigned long last_detected_ms = 0;
    bool last_echo_valid = false;
};

// Storage is capacity-based; the active number is read from localSensors.count().
LocalSensorState localState[MAX_CONFIGURED_SENSORS];
unsigned long lastSensorSampleMs = 0;
unsigned long lastBuzzerToggleMs = 0;
bool buzzerOn = false;
WarningLevel lastPrintedWarning = WARNING_NONE;

const char *zoneName(Zone zone) {
    switch (zone) {
        case ZONE_LEFT: return "LEFT";
        case ZONE_RIGHT: return "RIGHT";
        case ZONE_REAR: return "REAR";
        case ZONE_FRONT: return "FRONT";
        case ZONE_FRONT_LEFT: return "FRONT-LEFT";
        case ZONE_FRONT_RIGHT: return "FRONT-RIGHT";
        case ZONE_REAR_LEFT: return "REAR-LEFT";
        case ZONE_REAR_RIGHT: return "REAR-RIGHT";
        default: return "UNASSIGNED";
    }
}

WarningLevel levelForDistance(uint16_t distance) {
    if (distance <= DANGER_DISTANCE_CM) return WARNING_DANGER;
    if (distance <= CAUTION_DISTANCE_CM) return WARNING_CAUTION;
    return WARNING_NONE;
}

// Read the configured maximum from the same array used to initialize SensorManager.
uint16_t localConfigMaxDistance(size_t index) {
    return RECEIVER_SENSOR_CONFIG[index].detection_max_cm;
}

WarningLevel evaluateLocalSensors(unsigned long now) {
    WarningLevel highest = WARNING_NONE;
    for (size_t i = 0; i < localSensors.count(); ++i) {
        LocalSensorState &state = localState[i];
        if (state.has_recent_detection &&
            now - state.last_detected_ms > LOCAL_SENSOR_STALE_MS) {
            state.has_recent_detection = false;
            Serial.printf("LOCAL SENSOR %u | Detection expired; no recent object.\n",
                          static_cast<unsigned>(i + 1));
        }
        if (state.has_recent_detection) {
            const WarningLevel level = levelForDistance(state.distance_cm);
            if (level > highest) highest = level;
        }
    }
    return highest;
}

void sampleOneLocalSensor(unsigned long now) {
    if (localSensors.count() == 0) return;

    const size_t index = localSensors.nextIndex();
    SensorReading reading = {};
    const bool valid = localSensors.read(index, &reading);
    localSensorHealth[index].reportReading(valid);

    if (!valid) {
        // A no-echo reading is not proof that the obstacle disappeared.
        if (localSensorHealth[index].faultSuspected()) {
            Serial.printf("SENSOR HEALTH | Sensor %u (%s) may have a wiring/echo issue; "
                          "retaining last detection temporarily.\n",
                          static_cast<unsigned>(index + 1), zoneName(reading.zone));
        } else {
            Serial.printf("LOCAL SENSOR %u | %-11s | No echo; previous state retained.\n",
                          static_cast<unsigned>(index + 1), zoneName(reading.zone));
        }
        return;
    }

    LocalSensorState &state = localState[index];
    state.last_echo_valid = true;

    if (reading.object_present) {
        state.has_recent_detection = true;
        state.distance_cm = reading.distance_cm;
        state.last_detected_ms = now;
        const WarningLevel level = levelForDistance(reading.distance_cm);
        const char *levelText = level == WARNING_DANGER ? "DANGER" :
                                level == WARNING_CAUTION ? "CAUTION" : "CLEAR";
        Serial.printf("LOCAL SENSOR %u | %-11s | %3u cm | %s\n",
                      static_cast<unsigned>(index + 1), zoneName(reading.zone),
                      reading.distance_cm, levelText);
    } else {
        state.has_recent_detection = false;
        Serial.printf("LOCAL SENSOR %u | %-11s | Clear (no object within %u cm)\n",
                      static_cast<unsigned>(index + 1), zoneName(reading.zone),
                      static_cast<unsigned>(localConfigMaxDistance(index)));
    }
}

void printSummary(WarningLevel localLevel, WarningLevel remoteLevel,
                  WarningLevel combinedLevel) {
    Serial.println();
    Serial.println("============== BLINDLINK STATUS ==============");
    Serial.printf("Local sensors active : %u\n",
                  static_cast<unsigned>(localSensors.count()));
    Serial.printf("Remote objects active: %d\n", stateTable.getActiveObjectCount());
    Serial.printf("Local warning        : %s\n",
                  localLevel == WARNING_DANGER ? "DANGER" :
                  localLevel == WARNING_CAUTION ? "CAUTION" : "CLEAR");
    Serial.printf("Remote warning       : %s\n",
                  remoteLevel == WARNING_DANGER ? "DANGER" :
                  remoteLevel == WARNING_CAUTION ? "CAUTION" : "CLEAR");
    Serial.printf("OVERALL STATUS       : %s\n",
                  combinedLevel == WARNING_DANGER ? "DANGER - IMMEDIATE ATTENTION" :
                  combinedLevel == WARNING_CAUTION ? "CAUTION - OBJECT NEARBY" :
                  "CLEAR - NO CURRENT WARNING");
    Serial.println("==============================================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println();
    Serial.println("================================================");
    Serial.println("       BLINDLINK | VEHICLE B RECEIVER");
    Serial.println("================================================");
    Serial.println("LOCAL SENSOR = this vehicle's own HC-SR04 readings");
    Serial.println("REMOTE DATA  = detections received from another vehicle");
    Serial.println("------------------------------------------------");

    if (!localSensors.begin(RECEIVER_SENSOR_CONFIG, RECEIVER_SENSOR_COUNT)) {
        Serial.println("ERROR: Sensor configuration invalid or exceeds capacity.");
        return;
    }

    Serial.printf("Local sensors configured: %u\n",
                  static_cast<unsigned>(localSensors.count()));
    for (size_t i = 0; i < localSensors.count(); ++i) {
        Serial.printf("  Sensor %u: %-11s | TRIG GPIO %u | ECHO GPIO %u | range <= %u cm\n",
                      static_cast<unsigned>(i + 1),
                      zoneName(RECEIVER_SENSOR_CONFIG[i].zone),
                      RECEIVER_SENSOR_CONFIG[i].trig_pin,
                      RECEIVER_SENSOR_CONFIG[i].echo_pin,
                      RECEIVER_SENSOR_CONFIG[i].detection_max_cm);
    }

    if (!receiver.begin()) {
        Serial.println("ERROR: ESP-NOW receiver initialization failed.");
        return;
    }

    Serial.println("READY: Listening for remote vehicle data.");
    Serial.println("Tip: add/remove sensors in common/sensor_config.h only.");
    Serial.println();
}

void loop() {
    receiver.poll();
    const unsigned long now = millis();

    if (now - lastSensorSampleMs >= SENSOR_SAMPLE_GAP_MS) {
        lastSensorSampleMs = now;
        sampleOneLocalSensor(now);
    }

    const WarningLevel remoteLevel = warningManager.evaluateAll(stateTable);
    const WarningLevel localLevel = evaluateLocalSensors(now);
    const WarningLevel combinedLevel =
        (localLevel > remoteLevel) ? localLevel : remoteLevel;

    if (combinedLevel != lastPrintedWarning) {
        printSummary(localLevel, remoteLevel, combinedLevel);
        lastPrintedWarning = combinedLevel;
    }

    if (combinedLevel == WARNING_DANGER) {
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerOn = true;
    } else if (combinedLevel == WARNING_CAUTION) {
        const unsigned long interval = buzzerOn ? CAUTION_ON_MS : CAUTION_OFF_MS;
        if (now - lastBuzzerToggleMs >= interval) {
            lastBuzzerToggleMs = now;
            buzzerOn = !buzzerOn;
            digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
        }
    } else {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn = false;
        lastBuzzerToggleMs = now;
    }
    delay(5);
}

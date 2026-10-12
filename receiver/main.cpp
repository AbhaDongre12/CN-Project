
#include <Arduino.h>

#include "ReceiverStateTable.h"
#include "WarningManager.h"
#include "ReceiverCommunication.h"
#include "PriorityManager.h"

// Active buzzer wiring:
// Positive (+) -> GPIO25
// Negative (-) -> GND
constexpr uint8_t BUZZER_PIN = 25;

// Caution warning: short beep, followed by a longer pause.
constexpr unsigned long CAUTION_ON_MS = 180;
constexpr unsigned long CAUTION_OFF_MS = 650;

ReceiverStateTable stateTable;
WarningManager warningManager;
PriorityManager priorityManager;

ReceiverCommunication receiver(
    stateTable,
    warningManager,
    priorityManager);

unsigned long lastBuzzerToggleMs = 0;
bool buzzerOn = false;

WarningLevel lastPrintedWarning = WARNING_NONE;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println();
    Serial.println("======================================");
    Serial.println("       BLINDLINK LIVE RECEIVER");
    Serial.println("======================================");

    if (!receiver.begin())
    {
        Serial.println("Receiver initialization failed!");
        return;
    }

    Serial.println(
        "Receiver initialized. Waiting for packets...");
}

void loop()
{
    // Process incoming ESP-NOW messages and update receiver state.
    receiver.poll();

    // Determine the highest warning from active objects.
    const WarningLevel level =
        warningManager.evaluateAll(stateTable);

    const unsigned long now = millis();

    // Print only when the warning level changes.
    if (level != lastPrintedWarning)
    {
        warningManager.printWarning(level);
        lastPrintedWarning = level;
    }

    if (level == WARNING_DANGER)
    {
        // DANGER: continuous buzzer.
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerOn = true;
    }
    else if (level == WARNING_CAUTION)
    {
        // CAUTION: intermittent buzzer.
        const unsigned long interval =
            buzzerOn ? CAUTION_ON_MS : CAUTION_OFF_MS;

        if (now - lastBuzzerToggleMs >= interval)
        {
            lastBuzzerToggleMs = now;
            buzzerOn = !buzzerOn;

            digitalWrite(
                BUZZER_PIN,
                buzzerOn ? HIGH : LOW);
        }
    }
    else
    {
        // No warning: turn the buzzer off.
        digitalWrite(BUZZER_PIN, LOW);

        buzzerOn = false;
        lastBuzzerToggleMs = now;
    }

    delay(5);
}

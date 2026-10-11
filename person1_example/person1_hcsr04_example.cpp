#include <Arduino.h>
#include "../common/shared_types.h"
#include "../common/sensor_interface.h"
#include "../common/object_tracker.h"
#include "../common/sensor_health_monitor.h"

// Example pin assignments for ESP32-WROOM-32. Change to match your wiring.
// Avoid ESP32 boot-strapping pins for these signals where possible.
constexpr uint8_t HC_SR04_TRIG_PIN = 5;
constexpr uint8_t HC_SR04_ECHO_PIN = 18; // MUST receive ECHO through divider
constexpr Zone THIS_SENSOR_ZONE = ZONE_LEFT;
constexpr uint16_t DETECTION_MAX_CM = 250;
constexpr unsigned long SAMPLE_INTERVAL_MS = 70;
constexpr unsigned long STATE_INTERVAL_MS = 200;

SensorInterface sensor(HC_SR04_TRIG_PIN, HC_SR04_ECHO_PIN, THIS_SENSOR_ZONE, DETECTION_MAX_CM);
ObjectTracker tracker;
SensorHealthMonitor sensorHealth;
unsigned long lastSampleMs = 0;
unsigned long lastStateMs = 0;

void setup() {
  Serial.begin(115200);
  sensor.begin();
  Serial.println("BlindLink Person 1 HC-SR04 node starting");
}

void loop() {
  const unsigned long now = millis();
  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    SensorReading reading;
    const bool valid = sensor.read(&reading);
    sensorHealth.reportReading(valid);
    tracker.update(reading, now);
    if (sensorHealth.faultSuspected()) {
      Serial.println("WARNING: repeated HC-SR04 timeouts; sensor fault suspected or no echo available");
    }
  }

  if (now - lastStateMs >= STATE_INTERVAL_MS) {
    lastStateMs = now;
    TrackedObject active[MAX_OBJECTS];
    uint8_t expired[MAX_OBJECTS];
    const uint8_t activeCount = tracker.getActiveObjects(active);
    const uint8_t expiredCount = tracker.getExpiredObjects(expired);

    // This is the exact interface Person 2 consumes. Replace these Serial
    // prints with calls into Person 2's sender pipeline during integration.
    for (uint8_t i = 0; i < activeCount; ++i) {
      Serial.printf("ACTIVE id=%u distance_cm=%u zone=%u\n",
                    active[i].object_id, active[i].distance_cm,
                    static_cast<uint8_t>(active[i].zone));
    }
    for (uint8_t i = 0; i < expiredCount; ++i) {
      Serial.printf("EXPIRED/CLEAR id=%u\n", expired[i]);
    }
  }
}

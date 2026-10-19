
#pragma once

#include <Arduino.h>
#include "shared_types.h"

struct SensorConfig
{
    uint8_t trig_pin;
    uint8_t echo_pin;
    Zone zone;
    uint16_t detection_max_cm;
};

constexpr size_t MAX_CONFIGURED_SENSORS = 8;

// VEHICLE A — SENDER: 5 sensors
constexpr SensorConfig SENDER_SENSOR_CONFIG[] = {
    {5, 18, ZONE_FRONT_LEFT, 250},
    {19, 21, ZONE_FRONT, 250},

    // Replace these example GPIOs with your actual wiring.
    {27, 26, ZONE_REAR_LEFT, 250},
    {17, 16, ZONE_FRONT_RIGHT, 250},
    {33, 32, ZONE_REAR_RIGHT, 250}};

constexpr size_t SENDER_SENSOR_COUNT =
    sizeof(SENDER_SENSOR_CONFIG) / sizeof(SENDER_SENSOR_CONFIG[0]);

// VEHICLE B — RECEIVER: 1 local sensor
constexpr SensorConfig RECEIVER_SENSOR_CONFIG[] = {
    {5, 18, ZONE_FRONT, 250}};

constexpr size_t RECEIVER_SENSOR_COUNT =
    sizeof(RECEIVER_SENSOR_CONFIG) / sizeof(RECEIVER_SENSOR_CONFIG[0]);

static_assert(SENDER_SENSOR_COUNT <= MAX_CONFIGURED_SENSORS,
              "Sender sensor config exceeds MAX_CONFIGURED_SENSORS");

static_assert(RECEIVER_SENSOR_COUNT <= MAX_CONFIGURED_SENSORS,
              "Receiver sensor config exceeds MAX_CONFIGURED_SENSORS");

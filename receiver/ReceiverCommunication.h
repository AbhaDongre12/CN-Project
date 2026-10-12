
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <string.h>

#include "ReceiverProtocol.h"
#include "ReceiverStateTable.h"
#include "WarningManager.h"
#include "PriorityManager.h"

class ReceiverCommunication
{
public:
    static constexpr size_t PACKET_SIZE = 9;
    static constexpr uint8_t QUEUE_CAPACITY = 16;
    static constexpr uint8_t HEARTBEAT_SLOTS = 10;

    ReceiverCommunication(
        ReceiverStateTable &stateTable,
        WarningManager &warningManager,
        PriorityManager &priorityManager);

    bool begin();

    // Process queued ESP-NOW packets from loop().
    void poll();

    // Preserve the existing public API for direct processing/tests.
    void processMessage(
        const BlindLinkMessage &message);

private:
    ReceiverStateTable &stateTable;
    WarningManager &warningManager;
    PriorityManager &priorityManager;

    // Singleton instance used by the ESP-NOW callback.
    static ReceiverCommunication *instance;

    struct QueuedPacket
    {
        uint8_t bytes[PACKET_SIZE];
    };

    QueuedPacket queue[QUEUE_CAPACITY];

    volatile uint8_t queueRead = 0;
    volatile uint8_t queueWrite = 0;

    portMUX_TYPE queueMux = portMUX_INITIALIZER_UNLOCKED;

    // Per-vehicle heartbeat sequence tracking.
    uint8_t heartbeatVehicleIds[HEARTBEAT_SLOTS] = {};
    uint8_t heartbeatSequences[HEARTBEAT_SLOTS] = {};
    bool heartbeatSeen[HEARTBEAT_SLOTS] = {};

    static void onDataRecv(
        const uint8_t *mac,
        const uint8_t *data,
        int len);

    void enqueue(
        const uint8_t *data,
        int len);

    bool decode(
        const uint8_t *data,
        size_t len,
        BlindLinkMessage &message);

    bool acceptHeartbeat(
        const BlindLinkMessage &message);

    bool sendAck(
        const BlindLinkMessage &message);

    bool sendWirePacket(
        const uint8_t packet[PACKET_SIZE]);

    void handleMessage(
        const BlindLinkMessage &message);
};

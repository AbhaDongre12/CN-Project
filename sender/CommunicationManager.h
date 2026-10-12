
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "MessageComposer.h"

class CommunicationManager
{
public:
    CommunicationManager();

    bool begin(uint8_t vehicle_id);
    bool sendMessage(const BlindLinkMessage &message);
    bool sendClear(uint8_t object_id);
    bool sendHeartbeat();
    bool sendAck(uint8_t object_id, uint8_t acknowledged_sequence);
    bool sendReliableMessage(
        const BlindLinkMessage &message,
        uint8_t maxRetries = 3,
        unsigned long ackTimeoutMs = 300);
    bool hasAck();
    BlindLinkMessage getAck();
    void clearAck();
    bool isAckFor(uint8_t object_id, uint8_t sequence_number);

    void displayMacAddress();

private:
    static CommunicationManager *instance;

    static void onDataRecv(
        const uint8_t *mac,
        const uint8_t *data,
        int len);

    bool serializeMessage(
        const BlindLinkMessage &message,
        uint8_t *buffer,
        size_t &length);

    uint8_t broadcastAddress[6];
    uint8_t vehicle_id;
    MessageComposer messageComposer;

    volatile bool ack_received;
    BlindLinkMessage received_ack;
};

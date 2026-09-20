#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "MessageComposer.h"

class CommunicationManager{
public:
    CommunicationManager();

    bool begin();
    bool sendMessage(const BlindLinkMessage& message);
    bool sendClear(uint8_t object_id);
    bool sendHeartbeat();
    void displayMacAddress();

private:
    uint8_t broadcastAddress[6];
};

#include "CommunicationManager.h"
#include <cstring>

CommunicationManager *CommunicationManager::instance = nullptr;

static const uint8_t WIRE_FULL = 0x01;
static const uint8_t WIRE_DELTA = 0x02;
static const uint8_t WIRE_CLEAR = 0x03;
static const uint8_t WIRE_HEARTBEAT = 0x04;
static const uint8_t WIRE_ACK = 0x05;

CommunicationManager::CommunicationManager()
    : ack_received(false),
      received_ack{},
      messageComposer(0)
{
    for (int i = 0; i < 6; i++)
    {
        broadcastAddress[i] = 0xFF;
    }

    vehicle_id = 0;
}

bool CommunicationManager::begin(uint8_t id)
{
    vehicle_id = id;
    messageComposer = MessageComposer(id);
    instance = this;

    clearAck();

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW initialization failed.");
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};

    for (int i = 0; i < 6; i++)
    {
        peerInfo.peer_addr[i] = broadcastAddress[i];
    }

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(broadcastAddress))
    {
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            Serial.println("Failed to add broadcast peer.");
            return false;
        }
    }

    return true;
}

bool CommunicationManager::serializeMessage(
    const BlindLinkMessage &message,
    uint8_t *buffer,
    size_t &length)
{
    if (buffer == nullptr)
    {
        length = 0;
        return false;
    }

    for (int i = 0; i < 9; i++)
    {
        buffer[i] = 0;
    }

    buffer[1] = message.vehicle_id;
    buffer[2] = message.object_id;
    buffer[8] = message.sequence_number;

    switch (message.type)
    {
    case MESSAGE_FULL:
        buffer[0] = WIRE_FULL;
        buffer[3] = message.changed_fields;
        buffer[4] = message.distance_cm & 0xFF;
        buffer[5] = (message.distance_cm >> 8) & 0xFF;
        buffer[6] = static_cast<uint8_t>(message.zone);
        buffer[7] = 0;
        length = 9;
        return true;

    case MESSAGE_DELTA:
        buffer[0] = WIRE_DELTA;
        buffer[3] = message.changed_fields;

        if (message.changed_fields & FIELD_DISTANCE)
        {
            buffer[4] = message.distance_cm & 0xFF;
            buffer[5] = (message.distance_cm >> 8) & 0xFF;
        }

        if (message.changed_fields & FIELD_ZONE)
        {
            buffer[6] = static_cast<uint8_t>(message.zone);
        }

        length = 9;
        return true;

    case MESSAGE_CLEAR:
        buffer[0] = WIRE_CLEAR;
        length = 9;
        return true;

    case MESSAGE_HEARTBEAT:
        buffer[0] = WIRE_HEARTBEAT;
        length = 9;
        return true;

    case MESSAGE_ACK:
        buffer[0] = WIRE_ACK;
        length = 9;
        return true;

    case MESSAGE_SUPPRESS:
    default:
        length = 0;
        return false;
    }
}

bool CommunicationManager::sendMessage(
    const BlindLinkMessage &message)
{
    uint8_t buffer[16] = {};
    size_t length = 0;

    if (!serializeMessage(message, buffer, length) || length == 0)
    {
        Serial.println("Message serialization failed.");
        return false;
    }

    return esp_now_send(
               broadcastAddress,
               buffer,
               length) == ESP_OK;
}

bool CommunicationManager::sendReliableMessage(
    const BlindLinkMessage &message,
    uint8_t maxRetries,
    unsigned long ackTimeoutMs)
{
    if (message.type == MESSAGE_ACK ||
        message.type == MESSAGE_SUPPRESS ||
        message.sequence_number == 0)
    {
        Serial.println("Invalid message for reliable delivery.");
        return false;
    }

    // Clear any stale ACK once, before the first transmission.
    // Do not clear it on each retry: a delayed ACK for this same
    // object and sequence is still valid.
    clearAck();

    for (uint8_t attempt = 0; attempt <= maxRetries; attempt++)
    {
        Serial.printf(
            "Sending object=%u seq=%u (attempt %u/%u)\n",
            message.object_id,
            message.sequence_number,
            attempt + 1,
            maxRetries + 1);

        if (!sendMessage(message))
        {
            Serial.println("ESP-NOW send submission failed.");
            delay(10);
            continue;
        }

        unsigned long start = millis();

        while (millis() - start < ackTimeoutMs)
        {
            if (isAckFor(
                    message.object_id,
                    message.sequence_number))
            {
                Serial.println("Matching ACK confirmed.");
                return true;
            }

            delay(1);
        }

        Serial.println("ACK timeout.");
    }

    Serial.println("Message delivery unconfirmed.");
    return false;
}

bool CommunicationManager::sendClear(uint8_t object_id)
{
    BlindLinkMessage message =
        messageComposer.composeClear(object_id);

    return sendMessage(message);
}

bool CommunicationManager::sendHeartbeat()
{
    BlindLinkMessage message =
        messageComposer.composeHeartbeat();

    return sendMessage(message);
}

bool CommunicationManager::sendAck(
    uint8_t object_id,
    uint8_t acknowledged_sequence)
{
    BlindLinkMessage message = {};
    message.type = MESSAGE_ACK;
    message.vehicle_id = vehicle_id;
    message.object_id = object_id;
    message.sequence_number = acknowledged_sequence;

    return sendMessage(message);
}

void CommunicationManager::onDataRecv(
    const uint8_t *mac,
    const uint8_t *data,
    int len)
{
    if (instance == nullptr ||
        mac == nullptr ||
        data == nullptr ||
        len != 9)
    {
        return;
    }

    // Ignore packets that are not ACKs.
    if (data[0] != WIRE_ACK)
    {
        return;
    }

    CommunicationManager *self = instance;

    // The ACK must identify this sender's vehicle.
    if (data[1] != self->vehicle_id)
    {
        Serial.printf(
            "ACK ignored: vehicle mismatch (received=%u expected=%u)\n",
            data[1],
            self->vehicle_id);
        return;
    }

    // Do not filter against a hardcoded receiver MAC.
    // Record the ACK and let isAckFor() validate the object and sequence.
    portENTER_CRITICAL(&self->ackMux);

    self->received_ack = {};
    self->received_ack.type = MESSAGE_ACK;
    self->received_ack.vehicle_id = data[1];
    self->received_ack.object_id = data[2];
    self->received_ack.sequence_number = data[8];
    self->received_ack.changed_fields = FIELD_NONE;
    self->received_ack.distance_cm = 0;
    self->received_ack.zone = ZONE_LEFT;
    self->ack_received = true;

    portEXIT_CRITICAL(&self->ackMux);

    Serial.printf(
        "ACK RECEIVED from %02X:%02X:%02X:%02X:%02X:%02X"
        " | vehicle=%u object=%u seq=%u\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        data[1], data[2], data[8]);
}

bool CommunicationManager::hasAck()
{
    portENTER_CRITICAL(&ackMux);

    bool result = ack_received;

    portEXIT_CRITICAL(&ackMux);

    return result;
}

BlindLinkMessage CommunicationManager::getAck()
{
    portENTER_CRITICAL(&ackMux);

    BlindLinkMessage result = received_ack;

    ack_received = false;
    received_ack = {};

    portEXIT_CRITICAL(&ackMux);

    return result;
}

void CommunicationManager::clearAck()
{
    portENTER_CRITICAL(&ackMux);

    ack_received = false;
    received_ack = {};

    portEXIT_CRITICAL(&ackMux);
}

bool CommunicationManager::isAckFor(
    uint8_t object_id,
    uint8_t sequence_number)
{
    portENTER_CRITICAL(&ackMux);

    bool matched =
        ack_received &&
        received_ack.type == MESSAGE_ACK &&
        received_ack.vehicle_id == vehicle_id &&
        received_ack.object_id == object_id &&
        received_ack.sequence_number == sequence_number;

    if (matched)
    {
        ack_received = false;
        received_ack = {};
    }

    portEXIT_CRITICAL(&ackMux);

    return matched;
}

void CommunicationManager::displayMacAddress()
{
    Serial.print("Sender MAC: ");
    Serial.println(WiFi.macAddress());
}

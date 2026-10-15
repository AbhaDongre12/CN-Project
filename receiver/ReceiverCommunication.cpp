
#include "ReceiverCommunication.h"
#include <string.h>

ReceiverCommunication *ReceiverCommunication::instance = nullptr;

static const uint8_t WIRE_FULL = 0x01;
static const uint8_t WIRE_DELTA = 0x02;
static const uint8_t WIRE_CLEAR = 0x03;
static const uint8_t WIRE_HEARTBEAT = 0x04;
static const uint8_t WIRE_ACK = 0x05;

ReceiverCommunication::ReceiverCommunication(
    ReceiverStateTable &stateTable,
    WarningManager &warningManager,
    PriorityManager &priorityManager)
    : queueRead(0),
      queueWrite(0),
      stateTable(stateTable),
      warningManager(warningManager),
      priorityManager(priorityManager)
{
    memset(heartbeatVehicleIds, 0, sizeof(heartbeatVehicleIds));
    memset(heartbeatSequences, 0, sizeof(heartbeatSequences));
    memset(heartbeatSeen, 0, sizeof(heartbeatSeen));
}

bool ReceiverCommunication::begin()
{
    instance = this;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW initialization failed.");
        return false;
    }

    if (esp_now_register_recv_cb(onDataRecv) != ESP_OK)
    {
        Serial.println("Failed to register ESP-NOW callback.");
        esp_now_deinit();
        return false;
    }

    const uint8_t broadcast[6] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (!esp_now_is_peer_exist(broadcast))
    {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, broadcast, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            Serial.println("Could not add ESP-NOW broadcast peer.");
            esp_now_deinit();
            return false;
        }
    }

    Serial.print("ESP-NOW receiver ready. MAC: ");
    Serial.println(WiFi.macAddress());

    return true;
}

void ReceiverCommunication::onDataRecv(
    const uint8_t *mac,
    const uint8_t *data,
    int len)
{
    (void)mac;

    if (instance != nullptr && data != nullptr)
    {
        instance->enqueue(data, len);
    }
}

void ReceiverCommunication::enqueue(
    const uint8_t *data,
    int len)
{
    if (data == nullptr ||
        len != static_cast<int>(PACKET_SIZE))
    {
        Serial.printf(
            "Rejected packet length %d (expected 9).\n",
            len);
        return;
    }

    portENTER_CRITICAL(&queueMux);

    uint8_t next = static_cast<uint8_t>(
        (queueWrite + 1) % QUEUE_CAPACITY);

    if (next == queueRead)
    {
        portEXIT_CRITICAL(&queueMux);
        Serial.println("RX queue full; packet dropped.");
        return;
    }

    memcpy(queue[queueWrite].bytes, data, PACKET_SIZE);
    queueWrite = next;

    portEXIT_CRITICAL(&queueMux);
}

bool ReceiverCommunication::decode(
    const uint8_t *data,
    size_t len,
    BlindLinkMessage &message)
{
    if (data == nullptr || len != PACKET_SIZE)
        return false;

    switch (data[0])
    {
    case WIRE_FULL:
        message.type = MESSAGE_FULL;
        break;

    case WIRE_DELTA:
        message.type = MESSAGE_DELTA;
        break;

    case WIRE_CLEAR:
        message.type = MESSAGE_CLEAR;
        break;

    case WIRE_HEARTBEAT:
        message.type = MESSAGE_HEARTBEAT;
        break;

    default:
        Serial.printf(
            "Unknown wire message type 0x%02X.\n",
            data[0]);
        return false;
    }

    message.vehicle_id = data[1];
    message.object_id = data[2];
    message.changed_fields = data[3];

    // Distance is little-endian.
    message.distance_cm =
        static_cast<uint16_t>(data[4]) |
        (static_cast<uint16_t>(data[5]) << 8);

    message.zone = static_cast<Zone>(data[6]);
    message.sequence_number = data[8];

    if (message.type == MESSAGE_FULL ||
        message.type == MESSAGE_DELTA)
    {
        if ((message.changed_fields & ~0x03U) != 0)
            return false;

        if (message.type == MESSAGE_FULL &&
            (message.changed_fields & 0x03U) != 0x03U)
            return false;

        if (message.type == MESSAGE_DELTA &&
            message.changed_fields == 0)
            return false;

        // Validate only fields actually supplied by the sender.
        if ((message.changed_fields & 0x02U) != 0 &&
            message.zone > ZONE_FRONT)
            return false;

        if ((message.changed_fields & 0x01U) != 0 &&
            (message.distance_cm < 2 ||
             message.distance_cm > 600))
            return false;
    }

    return true;
}

void ReceiverCommunication::poll()
{
    while (true)
    {
        QueuedPacket packet;

        portENTER_CRITICAL(&queueMux);

        if (queueRead == queueWrite)
        {
            portEXIT_CRITICAL(&queueMux);
            break;
        }

        memcpy(packet.bytes, queue[queueRead].bytes, PACKET_SIZE);

        queueRead = static_cast<uint8_t>(
            (queueRead + 1) % QUEUE_CAPACITY);

        portEXIT_CRITICAL(&queueMux);

        // ACK is a response, not a state update.
        // Handle it directly because MESSAGE_ACK is not in the enum.
        if (packet.bytes[0] == WIRE_ACK)
        {
            Serial.printf(
                "ACK received: vehicle=%u object=%u seq=%u\n",
                packet.bytes[1],
                packet.bytes[2],
                packet.bytes[8]);

            continue;
        }

        BlindLinkMessage message = {};

        if (!decode(packet.bytes, PACKET_SIZE, message))
        {
            Serial.println("Malformed packet rejected.");
            continue;
        }

        bool accepted = false;

        if (message.type == MESSAGE_HEARTBEAT)
        {
            accepted = acceptHeartbeat(message);

            if (!accepted)
                Serial.println("Duplicate/stale heartbeat rejected.");
        }
        else
        {
            accepted = stateTable.processMessage(message);

            if (accepted)
                handleMessage(message);
        }

        if (accepted)
        {
            if (sendAck(message))
            {
                Serial.printf(
                    "ACK sent: vehicle=%u object=%u seq=%u\n",
                    message.vehicle_id,
                    message.object_id,
                    message.sequence_number);
            }
            else
            {
                Serial.println("ACK send failed.");
            }
        }
        else
        {
            Serial.println(
                "Message rejected; no success ACK sent.");
        }
    }

    stateTable.removeExpired(5000);
    warningManager.evaluateAll(stateTable);
}

bool ReceiverCommunication::acceptHeartbeat(
    const BlindLinkMessage &message)
{
    int freeSlot = -1;

    for (int i = 0; i < 10; ++i)
    {
        if (heartbeatSeen[i] &&
            heartbeatVehicleIds[i] == message.vehicle_id)
        {
            uint8_t difference = static_cast<uint8_t>(
                message.sequence_number -
                heartbeatSequences[i]);

            if (difference == 0 || difference >= 128)
                return false;

            heartbeatSequences[i] = message.sequence_number;
            return true;
        }

        if (!heartbeatSeen[i] && freeSlot < 0)
            freeSlot = i;
    }

    if (freeSlot < 0)
    {
        Serial.println("Heartbeat tracking table full.");
        return false;
    }

    heartbeatSeen[freeSlot] = true;
    heartbeatVehicleIds[freeSlot] = message.vehicle_id;
    heartbeatSequences[freeSlot] = message.sequence_number;

    return true;
}

bool ReceiverCommunication::sendAck(
    const BlindLinkMessage &message)
{
    uint8_t packet[PACKET_SIZE] = {};

    packet[0] = WIRE_ACK;
    packet[1] = message.vehicle_id;
    packet[2] = message.object_id;
    packet[8] = message.sequence_number;

    return sendWirePacket(packet);
}

bool ReceiverCommunication::sendWirePacket(
    const uint8_t packet[PACKET_SIZE])
{
    const uint8_t broadcast[6] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    return esp_now_send(
               broadcast,
               packet,
               PACKET_SIZE) == ESP_OK;
}

void ReceiverCommunication::processMessage(const BlindLinkMessage &message)
{
    // Apply the message to the receiver's state table first.
    bool accepted = stateTable.processMessage(message);

    if (!accepted)
    {
        Serial.println("Message rejected by state table.");
        return;
    }

    // Recalculate warnings and priority after a successful update.
    handleMessage(message);
}

void ReceiverCommunication::handleMessage(
    const BlindLinkMessage &message)
{
    (void)message;

    WarningLevel highestWarning =
        warningManager.evaluateAll(stateTable);

    NeighborObject *priorityObject =
        priorityManager.getHighestPriorityObject(
            stateTable,
            warningManager);

    if (priorityObject != nullptr)
    {
        Serial.print("Priority vehicle: ");
        Serial.println(priorityObject->vehicle_id);

        Serial.print("Priority object: ");
        Serial.println(priorityObject->object_id);

        Serial.print("Priority warning level: ");
        Serial.println(static_cast<int>(highestWarning));
    }
}

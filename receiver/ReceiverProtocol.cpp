
#include "ReceiverProtocol.h"

namespace
{
    constexpr size_t PACKET_SIZE = 9;

    constexpr uint8_t WIRE_FULL = 0x01;
    constexpr uint8_t WIRE_DELTA = 0x02;
    constexpr uint8_t WIRE_CLEAR = 0x03;
    constexpr uint8_t WIRE_HEARTBEAT = 0x04;
    constexpr uint8_t WIRE_ACK = 0x05;

    constexpr uint8_t FIELD_DISTANCE_MASK = 0x01;
    constexpr uint8_t FIELD_ZONE_MASK = 0x02;
    constexpr uint8_t VALID_FIELDS_MASK =
        FIELD_DISTANCE_MASK | FIELD_ZONE_MASK;
}

bool ReceiverProtocol::decode(
    const uint8_t *data,
    size_t len,
    BlindLinkMessage *message)
{
    if (data == nullptr || message == nullptr ||
        len != PACKET_SIZE)
    {
        return false;
    }

    const uint8_t wireType = data[0];

    // ACK packets are handled by the communication layer.
    if (wireType == WIRE_ACK)
    {
        return false;
    }

    if (wireType < WIRE_FULL ||
        wireType > WIRE_HEARTBEAT)
    {
        return false;
    }

    const uint8_t vehicleId = data[1];
    const uint8_t objectId = data[2];
    const uint8_t changedFields = data[3];

    if (vehicleId == 0)
    {
        return false;
    }

    if (wireType != WIRE_HEARTBEAT && objectId == 0)
    {
        return false;
    }

    if (wireType == WIRE_FULL &&
        changedFields != VALID_FIELDS_MASK)
    {
        return false;
    }

    if (wireType == WIRE_DELTA &&
        (changedFields == 0 ||
         (changedFields & ~VALID_FIELDS_MASK) != 0))
    {
        return false;
    }

    if ((wireType == WIRE_CLEAR ||
         wireType == WIRE_HEARTBEAT) &&
        changedFields != 0)
    {
        return false;
    }

    const uint16_t distance =
        static_cast<uint16_t>(data[4]) |
        (static_cast<uint16_t>(data[5]) << 8);

    const uint8_t zone = data[6];

    const bool carriesDistance =
        wireType == WIRE_FULL ||
        (wireType == WIRE_DELTA &&
         (changedFields & FIELD_DISTANCE_MASK) != 0);

    const bool carriesZone =
        wireType == WIRE_FULL ||
        (wireType == WIRE_DELTA &&
         (changedFields & FIELD_ZONE_MASK) != 0);

    if (carriesDistance &&
        (distance < 10 || distance > 600))
    {
        return false;
    }

    // Valid zone values are 0-7:
    // LEFT, RIGHT, REAR, FRONT,
    // FRONT_LEFT, FRONT_RIGHT, REAR_LEFT, REAR_RIGHT.
    if (carriesZone && zone > ZONE_REAR_RIGHT)
    {
        return false;
    }

    message->type =
        static_cast<MessageType>(wireType - 1);

    message->vehicle_id = vehicleId;
    message->object_id = objectId;
    message->changed_fields = changedFields;
    message->distance_cm = distance;
    message->zone = static_cast<Zone>(zone);
    message->sequence_number = data[8];

    return true;
}

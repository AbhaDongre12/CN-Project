
#pragma once

#include <Arduino.h>
#include "../common/shared_types.h"

enum MessageType
{
    MESSAGE_FULL,
    MESSAGE_DELTA,
    MESSAGE_CLEAR,
    MESSAGE_HEARTBEAT,
    MESSAGE_SUPPRESS
};

struct BlindLinkMessage
{
    MessageType type;

    uint8_t vehicle_id;
    uint8_t object_id;

    uint8_t changed_fields;

    uint16_t distance_cm;
    Zone zone;

    uint8_t sequence_number;
};

class ReceiverProtocol
{
public:
    static bool decode(
        const uint8_t *data,
        size_t len,
        BlindLinkMessage *message);
};


#pragma once
#include "ChangeDetector.h"

enum MessageType{
    MESSAGE_FULL,
    MESSAGE_DELTA,
    MESSAGE_CLEAR,
    MESSAGE_HEARTBEAT,
    MESSAGE_SUPPRESS,
    MESSAGE_ACK
};

enum ChangedField:uint8_t{
    FIELD_NONE=0,
    FIELD_DISTANCE=1,
    FIELD_ZONE=2
};

struct BlindLinkMessage{
    MessageType type;
    uint8_t vehicle_id;
    uint8_t object_id;
    uint8_t changed_fields;
    uint16_t distance_cm;
    Zone zone;
    uint8_t sequence_number;
};

class MessageComposer{
public:
    MessageComposer(uint8_t vehicle_id);

    BlindLinkMessage compose(const ChangeResult& result);
    BlindLinkMessage composeClear(uint8_t object_id);
    BlindLinkMessage composeHeartbeat();
    BlindLinkMessage composeAck(uint8_t object_id,uint8_t acknowledged_sequence);
    uint8_t getNextSequenceNumber();
    uint8_t getVehicleId() const;

private:
    uint8_t vehicle_id;
    uint8_t sequence_number;
};

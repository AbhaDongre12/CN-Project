#include "MessageComposer.h"

MessageComposer::MessageComposer(uint8_t id){
    vehicle_id=id;
    sequence_number=0;
}

BlindLinkMessage MessageComposer::compose(const ChangeResult& result){
    BlindLinkMessage message;

    message.vehicle_id=vehicle_id;
    message.object_id=result.object_id;
    message.sequence_number=getNextSequenceNumber();
    message.distance_cm=result.current_distance;
    message.zone=result.current_zone;
    message.changed_fields=0;

    if(result.type==NEW_OBJECT){
        message.type=MESSAGE_FULL;
        message.changed_fields=3;
    }
    else if(result.type==CHANGED_OBJECT){
        message.type=MESSAGE_DELTA;

        if(result.distance_changed){
            message.changed_fields|=1;
        }

        if(result.zone_changed){
            message.changed_fields|=2;
        }
    }
    else{
        message.type=MESSAGE_SUPPRESS;
    }

    return message;
}

BlindLinkMessage MessageComposer::composeClear(uint8_t object_id){
    BlindLinkMessage message;

    message.type=MESSAGE_CLEAR;
    message.vehicle_id=vehicle_id;
    message.object_id=object_id;
    message.changed_fields=0;
    message.distance_cm=0;
    message.zone=ZONE_LEFT;
    message.sequence_number=getNextSequenceNumber();

    return message;
}

BlindLinkMessage MessageComposer::composeHeartbeat(){
    BlindLinkMessage message;

    message.type=MESSAGE_HEARTBEAT;
    message.vehicle_id=vehicle_id;
    message.object_id=0;
    message.changed_fields=0;
    message.distance_cm=0;
    message.zone=ZONE_LEFT;
    message.sequence_number=getNextSequenceNumber();

    return message;
}

uint8_t MessageComposer::getNextSequenceNumber(){
    sequence_number++;

    if(sequence_number==0){
        sequence_number=1;
    }

    return sequence_number;
}
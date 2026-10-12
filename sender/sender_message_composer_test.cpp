
#include <iostream>
#include "MessageComposer.h"

void check(const char* name,bool passed){
    std::cout<<name<<": "<<(passed?"PASS":"FAIL")<<std::endl;
}

int main(){
    MessageComposer composer(10);
    ChangeResult result{};

    result.object_id=1;
    result.current_distance=250;
    result.previous_distance=0;
    result.current_zone=ZONE_LEFT;
    result.previous_zone=ZONE_LEFT;
    result.distance_changed=true;
    result.zone_changed=true;

    result.type=NEW_OBJECT;
    BlindLinkMessage message=composer.compose(result);
    check("TEST 1: NEW -> FULL",message.type==MESSAGE_FULL);
    check("TEST 2: FULL fields",message.changed_fields==(FIELD_DISTANCE|FIELD_ZONE));
    check("TEST 3: Vehicle ID",message.vehicle_id==10);
    check("TEST 4: FULL sequence",message.sequence_number!=0);

    result.type=CHANGED_OBJECT;
    result.current_distance=200;
    result.distance_changed=true;
    result.zone_changed=false;
    message=composer.compose(result);
    check("TEST 5: DISTANCE -> DELTA",message.type==MESSAGE_DELTA&&message.changed_fields==FIELD_DISTANCE);

    result.current_distance=250;
    result.current_zone=ZONE_RIGHT;
    result.distance_changed=false;
    result.zone_changed=true;
    message=composer.compose(result);
    check("TEST 6: ZONE -> DELTA",message.type==MESSAGE_DELTA&&message.changed_fields==FIELD_ZONE);

    result.distance_changed=true;
    result.zone_changed=true;
    message=composer.compose(result);
    check("TEST 7: BOTH -> DELTA",message.type==MESSAGE_DELTA&&message.changed_fields==(FIELD_DISTANCE|FIELD_ZONE));

    result.type=UNCHANGED_OBJECT;
    result.distance_changed=false;
    result.zone_changed=false;
    message=composer.compose(result);
    check("TEST 8: UNCHANGED -> SUPPRESS",message.type==MESSAGE_SUPPRESS);

    message=composer.composeClear(1);
    check("TEST 9: CLEAR",message.type==MESSAGE_CLEAR&&message.object_id==1);
    check("TEST 10: CLEAR sequence",message.sequence_number!=0);

    message=composer.composeHeartbeat();
    check("TEST 11: HEARTBEAT",message.type==MESSAGE_HEARTBEAT);
    check("TEST 12: HEARTBEAT sequence",message.sequence_number!=0);

    BlindLinkMessage ack=composer.composeAck(1,7);
    check("TEST 13: ACK",ack.type==MESSAGE_ACK&&ack.object_id==1&&ack.sequence_number==7);

    return 0;
}

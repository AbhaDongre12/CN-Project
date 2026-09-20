#include <Arduino.h>
#include "MessageComposer.h"

void testMessageComposer();

void setup(){
    Serial.begin(115200);
    delay(1000);
    testMessageComposer();
}

void loop(){
}

void testMessageComposer(){
    MessageComposer composer(10);

    TrackedObject object1={1,250,ZONE_LEFT};
    ChangeResult result;

    result.object_id=1;
    result.current_distance=250;
    result.previous_distance=0;
    result.current_zone=ZONE_LEFT;
    result.previous_zone=ZONE_LEFT;
    result.distance_changed=true;
    result.zone_changed=true;

    Serial.println("TEST 1: NEW -> FULL");

    result.type=NEW_OBJECT;

    BlindLinkMessage message=composer.compose(result);

    if(message.type==MESSAGE_FULL){
        Serial.println("PASS: FULL");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 2: DISTANCE CHANGED -> DELTA");

    result.type=CHANGED_OBJECT;
    result.current_distance=200;
    result.current_zone=ZONE_LEFT;
    result.distance_changed=true;
    result.zone_changed=false;

    message=composer.compose(result);

    if(message.type==MESSAGE_DELTA&&message.changed_fields==1){
        Serial.println("PASS: DISTANCE DELTA");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 3: ZONE CHANGED -> DELTA");

    result.current_distance=250;
    result.current_zone=ZONE_RIGHT;
    result.distance_changed=false;
    result.zone_changed=true;

    message=composer.compose(result);

    if(message.type==MESSAGE_DELTA&&message.changed_fields==2){
        Serial.println("PASS: ZONE DELTA");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 4: BOTH CHANGED -> DELTA");

    result.distance_changed=true;
    result.zone_changed=true;

    message=composer.compose(result);

    if(message.type==MESSAGE_DELTA&&message.changed_fields==3){
        Serial.println("PASS: BOTH DELTA");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 5: UNCHANGED -> SUPPRESS");

    result.type=UNCHANGED_OBJECT;
    result.distance_changed=false;
    result.zone_changed=false;

    message=composer.compose(result);

    if(message.type==MESSAGE_SUPPRESS){
        Serial.println("PASS: SUPPRESS");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 6: EXPIRED -> CLEAR");

    message=composer.composeClear(1);

    if(message.type==MESSAGE_CLEAR&&message.object_id==1){
        Serial.println("PASS: CLEAR");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 7: HEARTBEAT");

    message=composer.composeHeartbeat();

    if(message.type==MESSAGE_HEARTBEAT){
        Serial.println("PASS: HEARTBEAT");
    }
    else{
        Serial.println("FAIL");
    }
}
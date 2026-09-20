#include <Arduino.h>
#include "ChangeDetector.h"

void testChangeDetector();

void setup(){
    Serial.begin(115200);
    delay(1000);
    testChangeDetector();
}

void loop(){
}

void testChangeDetector(){
    StateTable stateTable;
    ChangeDetector detector(30);

    TrackedObject object1={1,250,ZONE_LEFT};

    ChangeResult result;

    Serial.println("TEST 1: NEW OBJECT");

    result=detector.detectChange(object1,stateTable);

    if(result.type==NEW_OBJECT){
        Serial.println("PASS: NEW");
    }
    else{
        Serial.println("FAIL");
    }

    stateTable.addObject(object1);

    Serial.println("TEST 2: UNCHANGED OBJECT");

    TrackedObject object2={1,270,ZONE_LEFT};

    result=detector.detectChange(object2,stateTable);

    if(result.type==UNCHANGED_OBJECT){
        Serial.println("PASS: UNCHANGED");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 3: CHANGED DISTANCE");

    TrackedObject object3={1,200,ZONE_LEFT};

    result=detector.detectChange(object3,stateTable);

    if(result.type==CHANGED_OBJECT){
        Serial.println("PASS: CHANGED");
    }
    else{
        Serial.println("FAIL");
    }

    Serial.println("TEST 4: CHANGED ZONE");

    TrackedObject object4={1,250,ZONE_RIGHT};

    result=detector.detectChange(object4,stateTable);

    if(result.type==CHANGED_OBJECT){
        Serial.println("PASS: CHANGED");
    }
    else{
        Serial.println("FAIL");
    }
}
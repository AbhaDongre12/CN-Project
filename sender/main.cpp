
#include <Arduino.h>
#include "CommunicationManager.h"
#include "ChangeDetector.h"
#include "StateTable.h"

CommunicationManager communicationManager;
StateTable stateTable;
ChangeDetector changeDetector(10);
MessageComposer messageComposer(1);

unsigned long lastSampleTime=0;
uint8_t sampleIndex=0;

const uint16_t simulatedDistances[]={200,180,150,145,120,100};
const uint8_t sampleCount=sizeof(simulatedDistances)/sizeof(simulatedDistances[0]);

void setup(){
    Serial.begin(115200);
    delay(1000);

    Serial.println("BlindLink Sender Starting...");

    stateTable.initialize();

    if(!communicationManager.begin(1)){
        Serial.println("ESP-NOW initialization failed!");
        return;
    }

    Serial.println("ESP-NOW initialized successfully.");
    communicationManager.displayMacAddress();
    Serial.println("Starting simulated object readings...");
}

void loop(){
    unsigned long now=millis();

    if(now-lastSampleTime<2000){
        return;
    }

    lastSampleTime=now;

    if(sampleIndex>=sampleCount){
        Serial.println("Simulation complete.");
        while(true){
            delay(1000);
        }
    }

    TrackedObject object;
    object.object_id=1;
    object.distance_cm=simulatedDistances[sampleIndex];
    object.zone=ZONE_LEFT;

    ChangeResult result=changeDetector.detectChange(object,stateTable);

    Serial.print("Simulated distance: ");
    Serial.print(object.distance_cm);
    Serial.println(" cm");

    if(result.type==NEW_OBJECT||result.type==CHANGED_OBJECT){
        BlindLinkMessage message={};
        message=messageComposer.compose(result);

        Serial.print("Message type: ");
        Serial.println(message.type);

        Serial.print("Object ID: ");
        Serial.println(message.object_id);

        Serial.print("Sequence number: ");
        Serial.println(message.sequence_number);

        Serial.print("Distance: ");
        Serial.print(message.distance_cm);
        Serial.println(" cm");

        if(communicationManager.sendMessage(message)){
            Serial.println("Message submitted to ESP-NOW.");
        }else{
            Serial.println("Message was not submitted.");
        }       

        stateTable.addObject(object);
        stateTable.updateConfirmedState(object.object_id,object);
    }else{
        Serial.println("No significant change detected.");
    }

    sampleIndex++;
}

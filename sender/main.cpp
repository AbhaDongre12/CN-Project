#include <Arduino.h>
#include "StateTable.h"

void testStateTable();

void setup(){
    Serial.begin(115200);
    delay(1000);
    testStateTable();
}

void loop(){
}

void testStateTable(){
    StateTable stateTable;

    TrackedObject object1={1,250,ZONE_LEFT};
    TrackedObject object2={2,400,ZONE_RIGHT};

    Serial.println("Adding objects...");

    stateTable.addObject(object1);
    stateTable.addObject(object2);

    stateTable.display();

    Serial.println("Finding Object 1...");

    int index=stateTable.findObject(1);
    Serial.print("Index: ");
    Serial.println(index);

    Serial.println("Updating Object 1...");

    TrackedObject updatedObject1={1,200,ZONE_LEFT};
    stateTable.updateObject(updatedObject1);

    stateTable.display();

    Serial.println("Removing Object 2...");

    stateTable.removeObject(2);

    stateTable.display();
}
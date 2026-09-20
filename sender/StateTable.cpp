#include "StateTable.h"

StateTable::StateTable(){
    initialize();
}

void StateTable::initialize(){
    for(int i=0;i<MAX_OBJECTS;i++){
        table[i].valid=false;
        table[i].object_id=0;
        table[i].distance_cm=0;
        table[i].zone=ZONE_LEFT;
    }
}

int StateTable::findObject(uint8_t object_id){
    for(int i=0;i<MAX_OBJECTS;i++){
        if(table[i].valid&&table[i].object_id==object_id){
            return i;
        }
    }
    return -1;
}

bool StateTable::addObject(const TrackedObject& object){
    for(int i=0;i<MAX_OBJECTS;i++){
        if(!table[i].valid){
            table[i].object_id=object.object_id;
            table[i].distance_cm=object.distance_cm;
            table[i].zone=object.zone;
            table[i].valid=true;
            return true;
        }
    }
    return false;
}

bool StateTable::updateObject(const TrackedObject& object){
    int index=findObject(object.object_id);

    if(index==-1){
        return false;
    }

    table[index].distance_cm=object.distance_cm;
    table[index].zone=object.zone;
    return true;
}

bool StateTable::removeObject(uint8_t object_id){
    int index=findObject(object_id);

    if(index==-1){
        return false;
    }

    table[index].valid=false;
    return true;
}

StoredState StateTable::getObject(int index){
    if(index<0||index>=MAX_OBJECTS){
        StoredState empty={0,0,ZONE_LEFT,false};
        return empty;
    }

    return table[index];
}

void StateTable::display(){
    for(int i=0;i<MAX_OBJECTS;i++){
        if(table[i].valid){
            Serial.print("ID: ");
            Serial.print(table[i].object_id);
            Serial.print(" Distance: ");
            Serial.print(table[i].distance_cm);
            Serial.print(" Zone: ");
            Serial.println(table[i].zone);
        }
    }
}
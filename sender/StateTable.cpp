
#include "StateTable.h"

StateTable::StateTable(){
    initialize();
}

void StateTable::initialize(){
    for(int i=0;i<MAX_OBJECTS;i++){
        table[i].local_state={0,0,ZONE_LEFT,false};
        table[i].confirmed_state={0,0,ZONE_LEFT,false};
        table[i].pending={false,0,0};
    }
}

int StateTable::findObject(uint8_t object_id){
    for(int i=0;i<MAX_OBJECTS;i++){
        if(table[i].local_state.valid&&table[i].local_state.object_id==object_id){
            return i;
        }
    }
    return -1;
}

bool StateTable::addObject(const TrackedObject& object){
    if(findObject(object.object_id)!=-1){
        return updateObject(object);
    }

    for(int i=0;i<MAX_OBJECTS;i++){
        if(!table[i].local_state.valid){
            table[i].local_state={object.object_id,object.distance_cm,object.zone,true};
            table[i].confirmed_state={0,0,ZONE_LEFT,false};
            table[i].pending={false,0,0};
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

    table[index].local_state.distance_cm=object.distance_cm;
    table[index].local_state.zone=object.zone;
    return true;
}

bool StateTable::removeObject(uint8_t object_id){
    int index=findObject(object_id);

    if(index==-1){
        return false;
    }

    table[index].local_state.valid=false;
    table[index].pending.active=false;
    return true;
}

StoredState StateTable::getObject(int index){
    if(index<0||index>=MAX_OBJECTS){
        return {0,0,ZONE_LEFT,false};
    }

    return table[index].local_state;
}

StoredState StateTable::getConfirmedObject(int index){
    if(index<0||index>=MAX_OBJECTS){
        return {0,0,ZONE_LEFT,false};
    }

    return table[index].confirmed_state;
}

ObjectState StateTable::getObjectState(int index){
    if(index<0||index>=MAX_OBJECTS){
        ObjectState empty;
        empty.local_state={0,0,ZONE_LEFT,false};
        empty.confirmed_state={0,0,ZONE_LEFT,false};
        empty.pending={false,0,0};
        return empty;
    }

    return table[index];
}

bool StateTable::updateConfirmedState(uint8_t object_id,const TrackedObject& object){
    int index=findObject(object_id);

    if(index==-1){
        return false;
    }

    table[index].confirmed_state={object.object_id,object.distance_cm,object.zone,true};
    return true;
}

bool StateTable::setPendingUpdate(uint8_t object_id,uint8_t sequence_number){
    int index=findObject(object_id);

    if(index==-1){
        return false;
    }

    table[index].pending.active=true;
    table[index].pending.sequence_number=sequence_number;
    table[index].pending.retry_count=0;
    return true;
}

bool StateTable::clearPendingUpdate(uint8_t object_id){
    int index=findObject(object_id);

    if(index==-1){
        return false;
    }

    table[index].pending.active=false;
    table[index].pending.retry_count=0;
    return true;
}

void StateTable::display(){
#ifdef ARDUINO
    for(int i=0;i<MAX_OBJECTS;i++){
        if(table[i].local_state.valid){
            Serial.print("ID: ");
            Serial.print(table[i].local_state.object_id);
            Serial.print(" Distance: ");
            Serial.print(table[i].local_state.distance_cm);
            Serial.print(" Zone: ");
            Serial.println(table[i].local_state.zone);
            Serial.print(" Confirmed: ");
            Serial.println(table[i].confirmed_state.valid);
            Serial.print(" Pending: ");
            Serial.println(table[i].pending.active);
        }
    }
#endif
}

#include "ChangeDetector.h"

ChangeDetector::ChangeDetector(uint16_t threshold){
    threshold_cm=threshold;
}

ChangeResult ChangeDetector::detectChange(const TrackedObject& currentObject,StateTable& stateTable){
    ChangeResult result;

    result.object_id=currentObject.object_id;
    result.current_distance=currentObject.distance_cm;
    result.current_zone=currentObject.zone;
    result.distance_changed=false;
    result.zone_changed=false;
    result.previous_distance=0;
    result.previous_zone=currentObject.zone;

    int index=stateTable.findObject(currentObject.object_id);

    if(index==-1){
        result.type=NEW_OBJECT;
        return result;
    }

    StoredState previousState=stateTable.getObject(index);

    result.previous_distance=previousState.distance_cm;
    result.previous_zone=previousState.zone;

    uint16_t distanceDifference;

    if(currentObject.distance_cm>=previousState.distance_cm){
        distanceDifference=currentObject.distance_cm-previousState.distance_cm;
    }
    else{
        distanceDifference=previousState.distance_cm-currentObject.distance_cm;
    }

    result.distance_changed=distanceDifference>=threshold_cm;
    result.zone_changed=currentObject.zone!=previousState.zone;

    if(result.distance_changed||result.zone_changed){
        result.type=CHANGED_OBJECT;
    }
    else{
        result.type=UNCHANGED_OBJECT;
    }

    return result;
}

void ChangeDetector::setThreshold(uint16_t threshold){
    threshold_cm=threshold;
}

uint16_t ChangeDetector::getThreshold(){
    return threshold_cm;
}

#include <iostream>
#include "ChangeDetector.h"

void check(const char* name,bool passed){
    std::cout<<name<<": "<<(passed?"PASS":"FAIL")<<std::endl;
}

int main(){
    StateTable stateTable;
    ChangeDetector detector(30);

    TrackedObject object1={1,250,ZONE_LEFT};
    ChangeResult result=detector.detectChange(object1,stateTable);

    check("TEST 1: NEW OBJECT",result.type==NEW_OBJECT);

    stateTable.addObject(object1);
    stateTable.updateConfirmedState(1,object1);

    TrackedObject object2={1,270,ZONE_LEFT};
    result=detector.detectChange(object2,stateTable);
    check("TEST 2: SMALL DISTANCE CHANGE",result.type==UNCHANGED_OBJECT);

    TrackedObject object3={1,200,ZONE_LEFT};
    result=detector.detectChange(object3,stateTable);
    check("TEST 3: LARGE DISTANCE CHANGE",result.type==CHANGED_OBJECT);

    TrackedObject object4={1,250,ZONE_RIGHT};
    result=detector.detectChange(object4,stateTable);
    check("TEST 4: ZONE CHANGE",result.type==CHANGED_OBJECT);

    return 0;
}

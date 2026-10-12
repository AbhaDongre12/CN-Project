
#include <iostream>
#include "StateTable.h"

int main(){
    StateTable stateTable;
    TrackedObject object1={1,250,ZONE_LEFT};
    TrackedObject object2={2,400,ZONE_RIGHT};

    std::cout<<"Test 1: Add objects"<<std::endl;
    bool added1=stateTable.addObject(object1);
    bool added2=stateTable.addObject(object2);
    std::cout<<"Object 1 added: "<<(added1?"PASS":"FAIL")<<std::endl;
    std::cout<<"Object 2 added: "<<(added2?"PASS":"FAIL")<<std::endl;

    std::cout<<"\nTest 2: Find Object 1"<<std::endl;
    int index=stateTable.findObject(1);
    std::cout<<"Index: "<<index<<std::endl;
    std::cout<<(index>=0?"PASS":"FAIL")<<std::endl;

    std::cout<<"\nTest 3: Update Object 1"<<std::endl;
    TrackedObject updatedObject1={1,200,ZONE_LEFT};
    bool updated=stateTable.updateObject(updatedObject1);
    StoredState updatedState=stateTable.getObject(index);
    std::cout<<"Update operation: "<<(updated?"PASS":"FAIL")<<std::endl;
    std::cout<<"Distance: "<<updatedState.distance_cm<<" cm"<<std::endl;
    std::cout<<(updatedState.distance_cm==200?"PASS":"FAIL")<<std::endl;

    std::cout<<"\nTest 4: Remove Object 2"<<std::endl;
    bool removed=stateTable.removeObject(2);
    std::cout<<"Remove operation: "<<(removed?"PASS":"FAIL")<<std::endl;
    std::cout<<"Object 2 lookup: "<<(stateTable.findObject(2)==-1?"PASS":"FAIL")<<std::endl;

    return 0;
}

//
// Created by dh on 08.11.20.
//

#include "TailRecAnalysis.h"
#include <map>

using namespace std;

void TailRecAnalysis::analyse(Model *htn) {
    map<int, set<int>*>* smaller = new map<int, set<int>*>;
    map<int, set<int>*>* smallerOrEq = new map<int, set<int>*>;
    for(int i = 0; i < htn->numMethods; i++) {
        int dec = htn->decomposedTask[i];
        for(int j =0; j < htn->numSubTasks[i]; j++) {
            int subt = htn->subTasks[i][j];
            if(j < (htn->numSubTasks[i] - 1)) {
                add(smaller, dec, subt);
            } else {
                add(smallerOrEq, dec, subt);
            }
        }
    }
}

void TailRecAnalysis::add(map<int, set<int> *> *map, int e1, int e2) {
    if(map->find(e1) == map->end()) {
        map->insert({e1, new set<int>});
    }
    map->at(e1)->insert(e2);
}

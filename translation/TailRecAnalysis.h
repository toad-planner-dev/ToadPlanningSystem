//
// Created by dh on 08.11.20.
//

#ifndef TOAD_TAILRECANALYSIS_H
#define TOAD_TAILRECANALYSIS_H


#include <iostream>
#include <map>
#include "../htnModel/Model.h"

class TailRecAnalysis {

public:
    void analyse(Model *htn);

    void add(map<int, set<int> *> *map, int dec, int subt);
};


#endif //TOAD_TAILRECANALYSIS_H

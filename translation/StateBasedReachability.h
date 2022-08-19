//
// Created by dh on 19.08.22.
//

#ifndef TOAD_STATEBASEDREACHABILITY_H
#define TOAD_STATEBASEDREACHABILITY_H

#include "../htnModel/Model.h"
#include <fst/fstlib.h>

using namespace fst;

class StateBasedReachability {
    const int Epsilon = -1;

    void addRule(StdVectorFst *fst, int from, int label, int to, int costs);
public:
    void statePruning(Model *htn, StdVectorFst *pFa);
    void statePruning2(Model *htn, StdVectorFst *pFa);
};


#endif //TOAD_STATEBASEDREACHABILITY_H

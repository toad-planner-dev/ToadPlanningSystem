//
// Created by dh on 16.10.20.
//

#ifndef TOSTRIPSAPPROXIMATION_RPGREACHABILITY_H
#define TOSTRIPSAPPROXIMATION_RPGREACHABILITY_H


#include "../htnModel/Model.h"
#include "../utils/IntPairHeap.h"
#include "../utils/IntStack.h"
#include "../translation/FiniteAutomaton.h"

class RPGReachability {
    const int UNREACHABLE = INT_MAX;

    Model* m;
    IntPairHeap* queue;
    int* hValPropInit;

    int* numSatPrecs;
    int* hValOp;
    int* hValProp;

    noDelIntSet markedFs;
    noDelIntSet markedOps;
    IntStack needToMark;

public:
    RPGReachability(progression::Model *htn);

    void computeReachability(FiniteAutomaton *dfa);
};


#endif //TOSTRIPSAPPROXIMATION_RPGREACHABILITY_H

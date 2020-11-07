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

public:
    RPGReachability(progression::Model *htn);

    void computeReachability(FiniteAutomaton *dfa);

    static void addToReachable(unordered_map<int, set<pair<int, int>> *> *dfa, int a, int from, int to);
};


#endif //TOSTRIPSAPPROXIMATION_RPGREACHABILITY_H

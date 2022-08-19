//
// Created by dh on 23.01.21.
//

#ifndef TOAD_DFSDELRELREACHABILITY_H
#define TOAD_DFSDELRELREACHABILITY_H


#include "htnModel/Model.h"

class dfsDelRelReachability {

public:
    void reachabilityAnalysis(FiniteAutomaton *dfa, progression::Model *htn);

    Model *htn;

    int calc(int i, set<int> *pSet);

    unordered_map<int, unordered_map<int, set<int> *> *> dfa;
};


#endif //TOAD_DFSDELRELREACHABILITY_H

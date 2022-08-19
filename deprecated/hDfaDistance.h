//
// Created by dh on 22.01.21.
//

#ifndef TOAD_HDFADISTANCE_H
#define TOAD_HDFADISTANCE_H


#include "translation/FiniteAutomaton.h"
#include "htnModel/Model.h"

class hDfaDistance {
private:
    unordered_map<int, unordered_map<int, int>*>* g;
    unordered_map<int, int>* hVals;
public:
    void write(const string& filename, FiniteAutomaton *dfa, progression::Model *htn);

    void generateInverseGraph(FiniteAutomaton *dfa, progression::Model *htn);

    void generateHeuVals();
};


#endif //TOAD_HDFADISTANCE_H

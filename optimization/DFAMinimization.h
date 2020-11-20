//
// Created by dh on 12.11.20.
//

#ifndef TOAD_DFAMINIMIZATION_H
#define TOAD_DFAMINIMIZATION_H


#include "../translation/FiniteAutomaton.h"
#include "../htnModel/Model.h"
class DFAMinimization {

public:
    void minimize(Model *htn, FiniteAutomaton *dfa_org, int S0, int F);

    int indexOf(vector<pair<int, set<int> *>> *pVector, int elem);

    void insert(vector<pair<int, set<int> *>> *pVector, int to);

    void sort(vector<pair<int, set<int> *>> *pVector, int i, int i1);

    int part(vector<pair<int, set<int> *>> *pVector, int l, int r);

    pair<set<int> *, set<int> *> getIntersection(set<int> *pSet, set<int> *pSet1);
};


#endif //TOAD_DFAMINIMIZATION_H

//
// Created by dh on 28.09.20.
//

#ifndef TOSTRIPSAPPROXIMATION_FINITEAUTOMATON_H
#define TOSTRIPSAPPROXIMATION_FINITEAUTOMATON_H

#include "Pair.h"
#include <set>
#include <unordered_map>
#include <vector>

using namespace std;

class FiniteAutomaton {

public:
    void addRule(int q0, int alpha, int q1);
    void print(string *pVector, int i, int i1);
    //unordered_map<Pair*, set<int>*, pairHash, pairComp> fda;
    unordered_map<int, unordered_map<int, set<int>*>*> fda2;
    unordered_map<int, set<pair<int, int>> *>* getActionMap();

    int stateID = 0; // next state id
    int startState = -1;
    int finalState = -1;
    int numTransitions = 0;
    bool actionMapDone = false;
    unordered_map<int, set<pair<int, int>> *>* actionMap;
};


#endif //TOSTRIPSAPPROXIMATION_FINITEAUTOMATON_H

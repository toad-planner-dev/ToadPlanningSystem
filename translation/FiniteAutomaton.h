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
    unordered_map<Pair*, set<int>*, pairHash, pairComp> fda;
    //unordered_map<int, unordered_map<int, set<int>*>> fda2;
};


#endif //TOSTRIPSAPPROXIMATION_FINITEAUTOMATON_H

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
    void print(std::vector<string> *pVector);
    unordered_map<Pair*, set<int>*, pairHash, pairComp> fda;
};


#endif //TOSTRIPSAPPROXIMATION_FINITEAUTOMATON_H

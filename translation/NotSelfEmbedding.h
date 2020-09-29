//
// Created by dh on 28.09.20.
//

#ifndef TOSTRIPSAPPROXIMATION_NOTSELFEMBEDDING_H
#define TOSTRIPSAPPROXIMATION_NOTSELFEMBEDDING_H

#include <list>
#include <vector>
#include "FiniteAutomaton.h"

using namespace std;

class NotSelfEmbedding {

public:
    int stateID = 0;
    int Epsilon = -1;
    int numPrim = 4;

    int left = 0;
    int right = 1;
    vector<int> *recursion;

    void addRule(std::vector<int> *pInt);
    vector<vector<int>*> rules;
    vector<vector<int>*> Nisets;
    FiniteAutomaton fa;

    void makeFA(int q0, vector<int> *alpha, int q1);
    void makeFA(int q0, int alpha, int q1);

    int isTerminalSym(int a);
    vector<int> * copySubSeq(vector<int> * in, int from, int to);

    vector<int> *Ni;

    int getNewState(vector<int> *NiS, vector<int> &qB, int C) const;
};


#endif //TOSTRIPSAPPROXIMATION_NOTSELFEMBEDDING_H

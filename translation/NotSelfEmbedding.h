//
// Created by dh on 28.09.20.
//

#ifndef TOSTRIPSAPPROXIMATION_NOTSELFEMBEDDING_H
#define TOSTRIPSAPPROXIMATION_NOTSELFEMBEDDING_H

#include <list>
#include <vector>
#include "FiniteAutomaton.h"

using namespace std;


struct grRule {
    int left = -1;
    int* right = nullptr;
    int rLength = 0;
    bool isLeftGenerating = false;
    bool isRightGenerating = false;
} ;


class NotSelfEmbedding {

public:
    const int recUnknown = 0;
    const int recLeft = 1;
    const int recRight = 2;
    const int recSelf = 3;
    const int recCycle = 4;

    const int Epsilon = -1;

    int stateID = 0; // next state id

    int numSymbols = 0;
    int numTerminals = 0;

    int* rFirst;
    int* rLast;

    int NumNis;   // how many sets?
    int* NiSize;  // size of single sets
    int **Ni;     // sets
    int *SymToNi; // mapping from symbol to Ni set
    int* NiRec;   // what kind of recursion is in set

    int maxRightHandside;
    int* qB;

    void addRule(std::vector<int> *pInt);
    void analyseRules();
    vector<grRule*> tempRules;

    int numRules = 0;
    grRule** rules = nullptr;

    FiniteAutomaton fa;

    void makeFA(int q0, vector<int> *alpha, int q1);
    void makeFA(int q0, int alpha, int q1);

    int isTerminalSym(int a);
    vector<int> * copySubSeq(grRule * in, int from, int to);


    int getNewState(int *NiS, int size, int* qB, int C) const;


    int comp(grRule *a, grRule *b);

    void sortRules();

    void printRule(grRule *rule) ;

    void determineRec(grRule *pRule);

};


#endif //TOSTRIPSAPPROXIMATION_NOTSELFEMBEDDING_H

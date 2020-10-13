//
// Created by dh on 28.09.20.
//

#ifndef TOSTRIPSAPPROXIMATION_CFGTOFDATRANSLATOR_H
#define TOSTRIPSAPPROXIMATION_CFGTOFDATRANSLATOR_H

#include <list>
#include <vector>
#include "FiniteAutomaton.h"
#include "../Model.h"

using namespace std;


struct grRule {
    int left = -1; // left-hand symbol
    int *right = nullptr; // right-hand symbols
    int rLength = 0; // length of right-hand side
    bool isLeftGenerating = false;
    bool isRightGenerating = false;
};


class CFGtoFDAtranslator {
    // temporal variables
    //int *qB; // variable used in algorithm to store newly added ids
    unordered_map<int, int> qB;
    vector<grRule *> tempRules;

    int isTerminalSym(int a);

    vector<int> *copySubSeq(grRule *in, int from, int to);

    //int getNewState(int *NiS, int size, int *qB, int C) const;

    int compareRules(grRule *a, grRule *b);

    void determineRuleRecursion(grRule *r);

    void sortRules();

public:
    // consts naming recursions of Ni
    const int recUnknown = 0;
    const int recLeft = 1;
    const int recRight = 2;
    const int recSelf = 3;
    const int recCycle = 4;

    // const for epsilon symbol
    const int Epsilon = -1;


    // grammar definition
    int numSymbols = 0;
    int numTerminals = 0;

    int numRules = 0;
    grRule **rules = nullptr;

    // for each non-terminal, the first and last rule where is it the left-hand side
    int *rFirst;
    int *rLast;

    // partitions
    int NumNis;   // number of partitions
    int *NiSize;  // size of single partition
    int **Ni;     // partitions
    int *SymToNi; // mapping from symbol to Ni it belongs tos
    int *NiRec;   // kind of recursion out of [recLeft, recRight, recSelf, recCycle]

    // resulting finite automaton
    FiniteAutomaton* dfa = new FiniteAutomaton;

    void addRule(std::vector<int> *pInt);

    void analyseRules();

    void makeFA(int q0, vector<int> *alpha, int q1);

    void makeFA(int q0, int alpha, int q1);

    void printRule(grRule *rule);

    bool isRegurlar;
};


#endif //TOSTRIPSAPPROXIMATION_CFGTOFDATRANSLATOR_H

//
// Created by dh on 17.03.21.
//

#ifndef TOAD_FA_H
#define TOAD_FA_H

#include <vector>
#include <set>
#include <unordered_map>
#include "TransitionContainer.h"

using namespace std;


class FA {


    // for minimization
    int *partitions;
    int *s2p;
    vector<int> firstI;
    vector<int> lastI;
    set<int> X;
    vector<int> inRest;


    void sortByPartition(int i, int i1);

    int partByPartition(int lo, int hi);

    void reachesAbyCtoX(int AStart, int AEnd, int c);

    bool XYintersectNotEq(int Y);

    int compByPartition(int i, int pivot);
    void qsSwap(int i, int j);

    void sortByIndex(int lo, int hi);

    int partByIndex(int lo, int hi);

    int compByIndex(int i, int j);

    void getEpsilonClosure(set<tStateID> *pSet);

public:
    TransitionContainer* delta;

    FA();
    ~FA();
    int numStates = 0;
    int numSymbols = 0;

    set<int> sInit;
    set<int> sGoal;

    void minimize();

    void compileToDFA();

    void printDOT();

    void printRules();

    void addRule(int from, int label, int to);

    void showDOT();

    void addSubFA(tStateID from, FA *pFa, tStateID to);

    void showDOT(string *pString);

    bool numStatesCorrect();

    bool isPrimitive(int numTerminals);

    bool isSorted(int y);

    void writeDfadHeuristic(string &file);

    void singleGoal();
};


#endif //TOAD_FA_H

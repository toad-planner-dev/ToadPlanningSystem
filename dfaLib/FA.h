//
// Created by dh on 17.03.21.
//

#ifndef TOAD_FA_H
#define TOAD_FA_H

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include "TransitionContainer.h"
#include <fst/fstlib.h>

using namespace std;
using namespace fst;


class FA {
public:
    StdVectorFst* fst;
    FA();
    ~FA();
    int numStates = 0;
    int numSymbols = 0;

    set<int> sInit;
    set<int> sGoal;

    void minimize();

    void compileToDFA();

    int nextState();

    int getNumStates();

//    void printDOT();

//    void printRules();
    void makeInit(int state);

    void makeFinal(int state);

    void addRule(int from, int label, int to);

    void showDOT();

    void addSubFA(tStateID from, FA *pFa, tStateID to);

    void showDOT(string *pString);

    bool numStatesCorrect();

    bool isPrimitive(int numTerminals);

    bool isSorted(int y);

//    void writeDfadHeuristic(string &file);
//
//    void singleGoal();
};


#endif //TOAD_FA_H

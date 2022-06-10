//
// Created by dh on 03.10.20.
//

#ifndef TOSTRIPSAPPROXIMATION_MODELWRITER_H
#define TOSTRIPSAPPROXIMATION_MODELWRITER_H


#include <fst/vector-fst.h>
#include "htnModel/Model.h"
#include "translation/FiniteAutomaton.h"

class ModelWriter {
    int epsilonAcs = 0;
    StringUtil su;
    Model *m;
    vector<int> dfaGoalStates;
    int finalState = -1;
    int multipleGoals = false;

    void writeDomain(ostream &os);

    void writeProblem(ostream &os, int startState, int goalState);

    void writePredDef(ostream &os, int maxState);

    void writeAction(ostream &os, int action, int dfaPrec, int dfaAdd, int dfaDel);

    void writeEpsilonAction(ostream& os, int prec, int add, int del);

public:
    void write(Model *htn, fst::VectorFst<fst::StdArc> *automaton, string dFile, string pFile);

    void writeActionCF(ostream& ostream, int action, set<pair<int, int>*>* cfSet);
    void writeEpsilonActionCF(ostream& os,  set<pair<int, int>*>* cfSet);

    bool getSASVal(int *store, int* somelist, int length) const;

    fst::VectorFst<fst::StdArc> *dfa;


    string sasCleanStr(string s);
};


#endif //TOSTRIPSAPPROXIMATION_MODELWRITER_H

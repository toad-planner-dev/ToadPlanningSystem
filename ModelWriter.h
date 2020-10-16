//
// Created by dh on 03.10.20.
//

#ifndef TOSTRIPSAPPROXIMATION_MODELWRITER_H
#define TOSTRIPSAPPROXIMATION_MODELWRITER_H


#include "htnModel/Model.h"
#include "translation/FiniteAutomaton.h"

class ModelWriter {
    int epsilonAcs = 0;
    StringUtil su;
    Model *m;

    void writeDomain(ostream &os);

    void writeProblem(ostream &os, int startState, int goalState);

    void writePredDef(ostream &os, int maxState);

    void writeAction(ostream &os, int action, int dfaPrec, int dfaAdd, int dfaDel);

    void writeEpsilonAction(ostream& os, int prec, int add, int del);

public:
    bool writePDDL = true;

    void write(Model *htn, FiniteAutomaton *automaton, string dFile, string pFile);

    void writeActionCF(ostream& ostream, int action, set<pair<int, int>*>* cfSet);
    void writeEpsilonActionCF(ostream& os,  set<pair<int, int>*>* cfSet);

    bool getSASVal(int *varPrec,int* l, int numVals, int action) const;

    FiniteAutomaton *dfa;

    void writeSASPlus(ostream &os, unordered_map<int, set<pair<int, int> *> *> &extraStuff);
};


#endif //TOSTRIPSAPPROXIMATION_MODELWRITER_H

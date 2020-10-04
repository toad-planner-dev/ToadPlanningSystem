//
// Created by dh on 03.10.20.
//

#ifndef TOSTRIPSAPPROXIMATION_MODELWRITER_H
#define TOSTRIPSAPPROXIMATION_MODELWRITER_H


#include "Model.h"
#include "translation/FiniteAutomaton.h"

class ModelWriter {
    int epsilonAcs = 0;
    FiniteAutomaton *dfa;
    StringUtil su;
    Model *m;

    void writeDomain(ostream &os);

    void writeProblem(ostream &os, int startState, int goalState);

    void writePredDef(ostream &os, int maxState);

    void writeAction(ostream &os, int action, int dfaPrec, int dfaAdd, int dfaDel);

    void writeEpsilonAction(ostream& os, int prec, int add, int del);

public:

    void write(Model *htn, FiniteAutomaton *automaton, string dFile, string pFile);

};


#endif //TOSTRIPSAPPROXIMATION_MODELWRITER_H

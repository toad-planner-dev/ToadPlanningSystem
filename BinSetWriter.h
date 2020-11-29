//
// Created by dh on 29.11.20.
//

#ifndef TOAD_BINSETWRITER_H
#define TOAD_BINSETWRITER_H


#include "htnModel/Model.h"
#include "translation/FiniteAutomaton.h"

class BinSetWriter {

public:
    void write(Model *htn, FiniteAutomaton *automaton, bool writePDDL, string dName, string pName);

    Model *m;
    FiniteAutomaton *dfa;
    bool getSASVal(int *store, int* somelist, int length) const;

    string sasCleanStr(string basicString);
};


#endif //TOAD_BINSETWRITER_H

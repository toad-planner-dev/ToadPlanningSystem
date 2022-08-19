//
// Created by dh on 26.11.20.
//

#ifndef TOAD_CHAINWRITER_H
#define TOAD_CHAINWRITER_H


#include "htnModel/Model.h"
#include "macroAction.h"

class ChainWriter {

public:
    void write(Model *htn, FiniteAutomaton *automaton, bool writePDDL, string dFile, string pFile);

    Model *m;
    FiniteAutomaton *dfa;

    void writeSASPlus(ostream &os);

    string sasCleanStr(string s);

    bool getSASVal(int *store, int* somelist, int length);

    bool isFinalState(int s);

    void printGraph(vector<macroAction *> *macroActions) const;

    void setTo(map<int, int> *someMap, int &key, int &val);
};


#endif //TOAD_CHAINWRITER_H

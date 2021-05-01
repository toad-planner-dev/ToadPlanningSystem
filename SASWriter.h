//
// Created by dh on 07.04.21.
//

#ifndef TOAD_SASWRITER_H
#define TOAD_SASWRITER_H

#include "htnModel/Model.h"
#include "dfaLib/FA.h"

class SASWriter {
    Model *m;

    string sasCleanStr(string s);
    bool getSASVal(int *store, int *somelist, int length);

    void getSASRepresentation(const bool *isBoolean, int **prev, int *numPrev, int **eff, int *numEff);

public:
    void write(Model *htn, FA *automaton, string dName, string pName);
};


#endif //TOAD_SASWRITER_H

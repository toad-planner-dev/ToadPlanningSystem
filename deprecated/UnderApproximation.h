//
// Created by dh on 27.11.20.
//

#ifndef TOAD_UNDERAPPROXIMATION_H
#define TOAD_UNDERAPPROXIMATION_H

#include <map>
#include "CFGtoFDAtranslator.h"

class UnderApproximation {
    void underapproximate(CFGtoFDAtranslator *g, Model *htn);

    grRule *rOneToOne(int from, int to);

    bool elem(int *pInt, int i, int i1);

    grRule *rOneToEpsilon(int from);
};

#endif //TOAD_UNDERAPPROXIMATION_H

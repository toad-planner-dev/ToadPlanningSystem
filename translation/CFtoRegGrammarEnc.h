//
// Created by dh on 20.11.20.
//

#ifndef TOAD_CFTOREGGRAMMARENC_H
#define TOAD_CFTOREGGRAMMARENC_H


#include "CFGtoFDAtranslator.h"

class CFtoRegGrammarEnc {

public:
    void underapproximate(CFGtoFDAtranslator *g, Model *htn);

    grRule *rOneToOne(int from, int to);

    bool elem(int *pInt, int i, int i1);

    void overapproximate(CFGtoFDAtranslator *pAtranslator, Model *pModel);

    grRule * rOneToEpsilon(int from);
};


#endif //TOAD_CFTOREGGRAMMARENC_H

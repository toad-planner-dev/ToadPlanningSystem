//
// Created by dh on 20.11.20.
//

#ifndef TOAD_CFTOREGGRAMMARENC_H
#define TOAD_CFTOREGGRAMMARENC_H


#include "CFGtoFDAtranslator.h"

class CFtoRegGrammarEnc {

public:

    grRule *rOneToOne(int from, int to);

    void overapproximate(CFGtoFDAtranslator *pAtranslator, Model *pModel);

    grRule * rOneToEpsilon(int from);
};


#endif //TOAD_CFTOREGGRAMMARENC_H

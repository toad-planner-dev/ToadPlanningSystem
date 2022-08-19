//
// Created by dh on 19.08.22.
//

#ifndef TOAD_HEUFADIST_H
#define TOAD_HEUFADIST_H


#include <fst/vector-fst.h>

using namespace fst;

class HeuFaDist {

public:
    void writeHeuristicLookupTable(VectorFst<StdArc> *fa);
};


#endif //TOAD_HEUFADIST_H

//
// Created by dh on 29.11.20.
//

#ifndef TOAD_TRIPLESET_H
#define TOAD_TRIPLESET_H

#include <set>

typedef __int128 TData;
//typedef uint64_t TData;
// typedef uint_fast64_t TData;
// typedef uint_least64_t TData;

using namespace std;

class TripleSet {
    /*
    const int lState = 24;
    const int lLabel = 16;
     */
    const int lState = 55;
    const int lLabel = 18;

    const int lShift = (lState * 2);
    const int sfShift = lState;
    const int stShift = 0;

    set<TData> *data;
    set<TData>::iterator it;
    TData out_sFrom;
    TData out_sTo;
    TData out_label;

    TData pSFrom;
    TData pSTo;

public:
    TripleSet();
    void add(uint32_t sFrom, uint32_t label, uint32_t sTo);

    void initIter();

    uint32_t getSFrom();

    uint32_t getSTo();

    uint32_t getSLabel();

    bool next();

    int size();
};


#endif //TOAD_TRIPLESET_H

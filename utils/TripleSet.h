//
// Created by dh on 29.11.20.
//

#ifndef TOAD_TRIPLESET_H
#define TOAD_TRIPLESET_H

//#include <set>
#include <vector>

//typedef __int128 TData;
typedef uint64_t TData;

using namespace std;

class TripleSet {
    const int numBits = sizeof(TData)* 8 / 2;

    vector<TData> *data;
    vector<TData>::iterator it;
    TData out_sFrom;
    TData out_sTo;
    int currentLabel;

public:
    TripleSet(int numLabels);

    void add(uint32_t sFrom, uint32_t label, uint32_t sTo);

    void initIter(int label);

    uint32_t getSFrom();

    uint32_t getSTo();

    bool next();

    int size();

    int numLabels;
    uint32_t pSTo;
};


#endif //TOAD_TRIPLESET_H

//
// Created by dh on 29.11.20.
//

#include <cstdint>
#include <cassert>
#include <math.h>
#include <iostream>
#include "TripleSet.h"

void TripleSet::add(uint32_t sFrom, uint32_t label, uint32_t sTo) {
    label++; // epsilon transitions
    assert(sFrom < pow(2, numBits + 1));
    assert(sTo < pow(2, numBits + 1));
    assert(label < numLabels);

    out_sFrom = sFrom;
    out_sTo = sTo;

    out_sFrom = out_sFrom << numBits;
    out_sTo = out_sTo;

    TData elem = out_sFrom | out_sTo;
    data[label].push_back(elem);
}

bool TripleSet::next() {
    if (this->it == data[currentLabel].end())
        return false;
    out_sFrom = *it;
    out_sTo = *it;

    out_sFrom = out_sFrom >> numBits;
    out_sTo = out_sTo & pSTo;
    ++it;
    return true;
}

void TripleSet::initIter(int label) {
    label++;
    this->currentLabel = label;
    assert(label < numLabels);
    it = data[label].begin();
}

uint32_t TripleSet::getSFrom() {
    return (uint32_t) this->out_sFrom;
}

uint32_t TripleSet::getSTo() {
    return (uint32_t) this->out_sTo;
}

TripleSet::TripleSet(int numLabels) {
    this->numLabels = numLabels + 1;
    this->data = new vector<TData>[numLabels + 1];
    this->it = data[0].end();

    pSTo = 0;
    pSTo = ~pSTo;
    pSTo = pSTo >> (numBits);
    pSTo = pSTo << (numBits);
}

int TripleSet::size() {
    int size = 0;
    for(int i = 0; i < numLabels; i++) {
        size += data[i].size();
    }
    return size;
}

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
    assert(sFrom < pow(2, lState + 1));
    assert(sTo < pow(2, lState + 1));
    assert(label < pow(2, lLabel + 1));

    out_label = label ;
    out_sFrom = sFrom;
    out_sTo = sTo;

    out_label = out_label << lShift;
    out_sFrom = out_sFrom << sfShift;
    out_sTo = out_sTo << stShift;

    TData elem = out_sFrom | out_sTo | out_label;
    this->data->insert(elem);
}

bool TripleSet::next() {
    if (this->it == data->end())
        return false;
    out_label = *it;
    out_sFrom = *it;
    out_sTo = *it;

    out_label = out_label >> lShift;
    out_label--;
    out_sFrom = out_sFrom & pSFrom;
    out_sFrom = out_sFrom >> lState;
    out_sTo = out_sTo & pSTo;
    ++it;
    return true;
}

void TripleSet::initIter() {
    it = data->begin();
}

uint32_t TripleSet::getSFrom() {
    return (uint32_t) this->out_sFrom;
}

uint32_t TripleSet::getSTo() {
    return (uint32_t) this->out_sTo;
}

uint32_t TripleSet::getSLabel() {
    return (uint32_t) this->out_label;
}

TripleSet::TripleSet() {
    this->data = new set<TData>;
    this->it = data->end();

    // build patterns for OR operation
    pSFrom = 0;
    pSFrom = ~pSFrom;
    pSFrom = pSFrom << lLabel;
    pSFrom = pSFrom >> (lLabel + lState);
    pSFrom = pSFrom << lState;

    pSTo = 0;
    pSTo = ~pSTo;
    pSTo = pSTo << (lLabel + lState);
    pSTo = pSTo >> (lLabel + lState);
}

int TripleSet::size() {
    return data->size();
}

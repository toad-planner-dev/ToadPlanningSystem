//
// Created by dh on 26.11.20.
//

#include "macroAction.h"

void macroAction::add(int a) {
    aSeq->push_back(a);
}

macroAction::macroAction(int sFrom) {
    this->sfrom = sFrom;
    this->aSeq = new vector<int>;
}

macroAction::~macroAction() {
    delete aSeq;
    delete precs;
    delete effects;
}

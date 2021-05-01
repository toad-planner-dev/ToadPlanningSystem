//
// Created by dh on 19.03.21.
//

#include <map>
#include "TransitionContainer.h"
#include <iostream>
#include <cassert>

TransitionContainer::TransitionContainer() {
    forward = new transitionFW;
    backward = new transitionBW;
}

TransitionContainer::~TransitionContainer() {
    freeFW();
    freeBW();
}

void TransitionContainer::freeFW() {
    // from -> (to -> label)
    if (forward == nullptr)
        return;

    for (auto from : *forward) {
        for (auto to : *from.second) {
            delete to.second;
        }
        delete from.second;
    }
    delete forward;
    forward = nullptr;
}

void TransitionContainer::freeBW() {
    // label -> (to -> from)
    if (backward == nullptr)
        return;

    for (auto label : *backward) {
        for (auto to : *label.second) {
            delete to.second;
        }
        delete label.second;
    }
    delete backward;
}

set<tStateID> *TransitionContainer::getFrom(int to, int label) {
    // structure of backward: label -> (to -> from)
    ensureBW();

    if (backward->find(label) != backward->end()) {
        auto temp = backward->at(label);
        if (temp->find(to) != temp->end()) {
            set<tStateID> *res = temp->at(to);
            return res;
        }
    }
    return nullptr;
}

void TransitionContainer::addRule(tStateID from, tLabelID alpha, tStateID to) {
    if ((alpha == epsilon) && (from == to)) // epsilon selfloop
        return;

    bool didIncrease = false;
    if (bwValid) {
        addToBW(from, alpha, to, didIncrease);
    }
    if (fwValid) {
        addToFW(from, alpha, to, didIncrease);
    }
}

void TransitionContainer::addToFW(tStateID from, tLabelID alpha, tStateID to, bool didIncrease) {

    // structure of forward: from -> (label -> to)
    unordered_map<tLabelID, set<tStateID> *> *labelToMap;
    auto posInFromSet = forward->find(from);
    if (posInFromSet != forward->end()) {
        labelToMap = posInFromSet->second;
    } else {
        labelToMap = new unordered_map<tLabelID, set<tStateID> *>;
        forward->insert({from, labelToMap});
    }

    set<tStateID> *toSet;
    auto posInLTMap = labelToMap->find(alpha);
    if (posInLTMap != labelToMap->end()) {
        toSet = posInLTMap->second;
    } else {
        toSet = new set<tStateID>;
        labelToMap->insert({alpha, toSet});
    }

    if (toSet->find(to) == toSet->end()) {
        if (!didIncrease) { // may have been increase in the update of "backward"
            numTransitions++;
        }
        toSet->insert(to);
    }
}

void TransitionContainer::addToBW(tStateID from, tLabelID alpha, tStateID to, bool &didIncrease) {

    // structure of backward: label -> (to -> from)
    unordered_map<tStateID, set<tStateID> *> *toFromMap;
    auto posInCSet = backward->find(alpha);
    if (posInCSet != backward->end()) {
        toFromMap = posInCSet->second;
    } else {
        toFromMap = new unordered_map<tStateID, set<tStateID> *>;
        backward->insert({alpha, toFromMap});
    }

    set<tStateID> *fromSet;
    auto posInTFMap = toFromMap->find(to);
    if (posInTFMap != toFromMap->end()) {
        fromSet = posInTFMap->second;
    } else {
        fromSet = new set<tStateID>;
        toFromMap->insert({to, fromSet});
    }

    if (fromSet->find(from) == fromSet->end()) {
        numTransitions++;
        fromSet->insert(from);
        didIncrease = true;
    }
}

void *TransitionContainer::indexTransformation(int *indexMapping) {
    transitionBW *bwTransformed = new transitionBW; // label -> (to -> from)
    tStateID from, to;
    tLabelID alpha;
    fullIterInit();
    int newTransitions = 0;
    while (fullIterNext(&from, &alpha, &to)) {
        from = indexMapping[from];
        to = indexMapping[to];
        // structure of backward: label -> (to -> from)
        unordered_map<tStateID, set<tStateID> *> *toFromMap;
        auto posInCSet = bwTransformed->find(alpha);
        if (posInCSet != bwTransformed->end()) {
            toFromMap = posInCSet->second;
        } else {
            toFromMap = new unordered_map<tStateID, set<tStateID> *>;
            bwTransformed->insert({alpha, toFromMap});
        }

        set<tStateID> *fromSet;
        auto posInTFMap = toFromMap->find(to);
        if (posInTFMap != toFromMap->end()) {
            fromSet = posInTFMap->second;
        } else {
            fromSet = new set<tStateID>;
            toFromMap->insert({to, fromSet});
        }

        if (fromSet->find(from) == fromSet->end()) {
            newTransitions++;
            fromSet->insert(from);
        }
    }
    freeBW();
    freeFW();
    backward = bwTransformed;
    this->numTransitions = newTransitions;

    bwValid = true;
    fwValid = false;
}

void TransitionContainer::outIterInit(tStateID from) {
    // from -> (to -> label)
    ensureFW();
    auto fromState = forward->find(from);
    if (fromState == forward->end()) {
        iterBlocked = true;
        return;
    } else {
        iterBlocked = false;
        iterLabel = fromState->second->begin();
        iterLabelEnd = fromState->second->end();

        iterToState = iterLabel->second->begin();
        iterToStateEnd = iterLabel->second->end();

        while (iterToState == iterToStateEnd) {
            ++iterLabel;
            if (iterLabel != iterLabelEnd) {
                iterToState = iterLabel->second->begin();
                iterToStateEnd = iterLabel->second->end();
            } else {
                iterBlocked = true;
                break;
            }
        }
    }
}

bool TransitionContainer::outIterNext(tLabelID *alpha, tStateID *to) {
    if (iterBlocked) {
        return false;
    } else {
        auto elem = *iterLabel;
        *alpha = elem.first;
        *to = *iterToState;
        ++iterToState;
        while (iterToState == iterToStateEnd) {
            ++iterLabel;
            if (iterLabel != iterLabelEnd) {
                iterToState = iterLabel->second->begin();
                iterToStateEnd = iterLabel->second->end();
            } else {
                iterBlocked = true;
                break;
            }
        }
        return true;
    }
}

bool iterBlocked2 = true;

transitionFW::iterator iterFrom;
transitionFW::iterator iterFromEnd;

innerFW::iterator iterLabel2;
innerFW::iterator iterLabelEnd2;

set<tStateID>::iterator iterTo2;
set<tStateID>::iterator iterToEnd2;

void TransitionContainer::fullIterInit() {
//    if (bwValid) {
//        cout << "implement full iter bw" << endl;
//    }

    ensureFW(); // from -> (label -> to)
    if (forward->empty()) {
        iterBlocked2 = true;
    } else {
        iterBlocked2 = false;
        iterFrom = forward->begin();
        iterFromEnd = forward->end();

        iterLabel2 = iterFrom->second->begin();
        iterLabelEnd2 = iterFrom->second->end();

        iterTo2 = iterLabel2->second->begin();
        iterToEnd2 = iterLabel2->second->end();
    }
}

bool TransitionContainer::fullIterNext(tStateID *from, tLabelID *label, tStateID *to) {
    // from -> (label -> to)
    if (iterBlocked2) {
        return false;
    } else {
        auto elem = *iterFrom;
        *from = elem.first;
        *label = iterLabel2->first;
        *to = *iterTo2;
        ++iterTo2;

        while (iterTo2 == iterToEnd2) {
            ++iterLabel2;
            if (iterLabel2 != iterLabelEnd2) {
                iterTo2 = iterLabel2->second->begin();
                iterToEnd2 = iterLabel2->second->end();
            } else {
                iterFrom++;
                if (iterFrom == iterFromEnd) {
                    iterBlocked2 = true;
                    break;
                } else {
                    iterLabel2 = iterFrom->second->begin();
                    iterLabelEnd2 = iterFrom->second->end();
                    iterTo2 = iterLabel2->second->begin();
                    iterToEnd2 = iterLabel2->second->end();
                }
            }
        }
        return true;
    }
}

void TransitionContainer::moveBWtoFW() {
    freeFW();
    numTransitions = 0;
    forward = new transitionFW;
    for (auto l : *backward) {
        for (auto to : *l.second) {
            for (auto from : *to.second) {
                addToFW(from, l.first, to.first, false);
            }
            delete to.second;
        }
        delete l.second;
    }
    delete backward;
    backward = nullptr;
    bwValid = false;
    fwValid = true;
}

void TransitionContainer::moveFWtoBW() {
    // forward: from -> (label -> to)
    // backward: label -> (to -> from)
    freeBW();
    numTransitions = 0;
    backward = new transitionBW;
    for (auto from : *forward) {
        for (auto lToMap : *from.second) {
            for (auto to : *lToMap.second) {
                bool dontcare;
                addToBW(from.first, lToMap.first, to,  dontcare);
                //cout << "from: " << from.first << " by: " << lToMap.first << " to: " << to << endl;
            }
            delete lToMap.second;
        }
        delete from.second;
    }
    delete forward;
    forward = nullptr;
    bwValid = true;
    fwValid = false;
}

void TransitionContainer::ensureBW() {
    if (!bwValid) {
        moveFWtoBW();
    }
}

void TransitionContainer::ensureFW() {
    if (!fwValid) {
        moveBWtoFW();
    }
}

bool iterBlocked3 = true;
set<tStateID>::iterator iterTo3;
set<tStateID>::iterator iterToEnd3;

void TransitionContainer::outIterInit(tStateID from, tLabelID label) {
    // from -> (label -> to)
    iterBlocked3 = true;
    ensureFW();
    auto fromPos = forward->find(from);
    if (fromPos != forward->end()) {
        auto ltMapPos = fromPos->second->find(label);
        if (ltMapPos != fromPos->second->end()) {
            iterTo3 = ltMapPos->second->begin();
            iterToEnd3 = ltMapPos->second->end();
            iterBlocked3 = false;
        }
    }
}

bool TransitionContainer::outIterNext(tStateID *to) {
    if (iterBlocked3) {
        return false;
    } else {
        *to = *iterTo3;
        ++iterTo3;
        if (iterTo3 == iterToEnd3) {
            iterBlocked3 = true;
        }
        return true;
    }
}

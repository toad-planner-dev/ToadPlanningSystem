//
// Created by dh on 19.03.21.
//

#include <climits>
#include <cassert>
#include <map>
#include "TransitionContainer.h"

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
    assert(bwValid);

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
    if ((alpha == -1) && (from == to)) // epsilon selfloop
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

    // structure of forward: from -> (to -> label)
    unordered_map<tStateID, set<tLabelID> *> *toLabelMap;
    auto posInFromSet = forward->find(from);
    if (posInFromSet != forward->end()) {
        toLabelMap = posInFromSet->second;
    } else {
        toLabelMap = new unordered_map<tStateID, set<tLabelID> *>;
        forward->insert({from, toLabelMap});
    }

    set<tLabelID> *labelSet;
    auto posInTLMap = toLabelMap->find(to);
    if (posInTLMap != toLabelMap->end()) {
        labelSet = posInTLMap->second;
    } else {
        labelSet = new set<tLabelID>;
        toLabelMap->insert({to, labelSet});
    }

    if (labelSet->find(alpha) == labelSet->end()) {
        if (!didIncrease) { // may have been increase in the update of "backward"
            numTransitions++;
        }
        labelSet->insert(alpha);
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

void *TransitionContainer::updateFAData(int *indexMapping) {
    transitionBW *bwTransformed = new transitionBW;
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
    backward = bwTransformed;
    this->numTransitions = newTransitions;
    copyBWtoFW();
}

void TransitionContainer::outIterInit(tStateID from) {
    // from -> (to -> label)
    assert(this->fwValid);
    auto fromState = forward->find(from);
    if (fromState == forward->end()) {
        iterBlocked = true;
        return;
    } else {
        iterBlocked = false;
        iterToState = fromState->second->begin();
        iterToStateEnd = fromState->second->end();

        iterLabel = iterToState->second->begin();
        iterLabelEnd = iterToState->second->end();

        while (iterLabel == iterLabelEnd) {
            ++iterToState;
            if (iterToState != iterToStateEnd) {
                iterLabel = iterToState->second->begin();
                iterLabelEnd = iterToState->second->end();
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
        auto elem = *iterToState;
        *to = elem.first;
        *alpha = *iterLabel;
        ++iterLabel;
        while (iterLabel == iterLabelEnd) {
            ++iterToState;
            if (iterToState != iterToStateEnd) {
                iterLabel = iterToState->second->begin();
                iterLabelEnd = iterToState->second->end();
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

innerFW::iterator iterToState2;
innerFW::iterator iterToStateEnd2;

set<tLabelID>::iterator iterLabel2;
set<tLabelID>::iterator iterLabelEnd2;

void TransitionContainer::fullIterInit() {
    // from -> (to -> label)
    assert(this->fwValid);
    if (forward->empty()) {
        iterBlocked2 = true;
    } else {
        iterBlocked2 = false;
        iterFrom = forward->begin();
        iterFromEnd = forward->end();

        iterToState2 = iterFrom->second->begin();
        iterToStateEnd2 = iterFrom->second->end();

        iterLabel2 = iterToState2->second->begin();
        iterLabelEnd2 = iterToState2->second->end();
    }
}

bool TransitionContainer::fullIterNext(tStateID *from, tLabelID *label, tStateID *to) {
    // from -> (to -> label)
    if (iterBlocked2) {
        return false;
    } else {
        auto elem = *iterFrom;
        *from = elem.first;
        *to = iterToState2->first;
        *label = *iterLabel2;
        ++iterLabel2;

        while (iterLabel2 == iterLabelEnd2) {
            ++iterToState2;
            if (iterToState2 != iterToStateEnd2) {
                iterLabel2 = iterToState2->second->begin();
                iterLabelEnd2 = iterToState2->second->end();
            } else {
                iterFrom++;
                if (iterFrom == iterFromEnd) {
                    iterBlocked2 = true;
                    break;
                } else {
                    iterToState2 = iterFrom->second->begin();
                    iterToStateEnd2 = iterFrom->second->end();
                    iterLabel2 = iterToState2->second->begin();
                    iterLabelEnd2 = iterToState2->second->end();
                }
            }
        }
        return true;
    }
}

transitionFW *tempforward = nullptr;
int tempTransitions;
void TransitionContainer::addTempArc(tStateID from, tLabelID label, tStateID to) {
    if (tempforward == tempforward) {
        tempforward = new transitionFW;
        tempTransitions = 0;
    }

    // structure of forward: from -> (to -> label)
    unordered_map<tStateID, set<tLabelID> *> *toLabelMap;
    auto posInFromSet = tempforward->find(from);
    if (posInFromSet != tempforward->end()) {
        toLabelMap = posInFromSet->second;
    } else {
        toLabelMap = new unordered_map<tStateID, set<tLabelID> *>;
        tempforward->insert({from, toLabelMap});
    }

    set<tLabelID> *labelSet;
    auto posInTLMap = toLabelMap->find(to);
    if (posInTLMap != toLabelMap->end()) {
        labelSet = posInTLMap->second;
    } else {
        labelSet = new set<tLabelID>;
        toLabelMap->insert({to, labelSet});
    }

    if (labelSet->find(label) == labelSet->end()) {
        tempTransitions++;
        labelSet->insert(label);
    }
}

void TransitionContainer::switchToTemp() {
    freeFW();
    freeBW();
    this->forward = tempforward;
    tempforward = nullptr;
    this->numTransitions = tempTransitions;
}

void TransitionContainer::copyBWtoFW() {
    //transitionFW *forward; // from -> (to -> label)
    //transitionBW *backward; // label -> (to -> from)
    freeFW();
    forward = new transitionFW;
    for (auto l : *backward) {
        for (auto to : *l.second) {
            for (auto from : *to.second) {
                addToFW(from, l.first, to.first, true);
            }
        }
    }
}

void TransitionContainer::copyFWtoBW() {
    //transitionFW *forward; // from -> (to -> label)
    //transitionBW *backward; // label -> (to -> from)
    int numTransitions = this->numTransitions;
    freeBW();
    backward = new transitionBW;
    for (auto from : *forward) {
        for (auto to : *from.second) {
            for (auto label : *to.second) {
                bool dontcare;
                addToBW(from.first, label, to.first, dontcare);
            }
        }
    }
    this->numTransitions = numTransitions;
}

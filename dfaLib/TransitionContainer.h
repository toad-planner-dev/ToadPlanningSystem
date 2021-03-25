//
// Created by dh on 19.03.21.
//

#ifndef TOAD_TRANSITIONCONTAINER_H
#define TOAD_TRANSITIONCONTAINER_H

#include <vector>
#include <set>
#include <unordered_map>

using namespace std;

// typedef
typedef unsigned int tStateID;
typedef unsigned short tLabelID;
typedef unordered_map<tStateID, set<tLabelID> *> innerFW;
typedef unordered_map<tStateID, set<tStateID> *> innerBW;
typedef unordered_map<tStateID, innerFW *> transitionFW;
typedef unordered_map<tLabelID, innerBW *> transitionBW;

class TransitionContainer {
    transitionFW *forward; // from -> (to -> label)
    transitionBW *backward; // label -> (to -> from)

    int numTransitions = 0;

    // - it is possible to either maintain both directions or just one
    // - by default, only the backward system is maintained (it might be copied to forward, *moved* to forward)
    // - the following variables show which one is valid, functions like "addRule" will only maintain those that are valid
    bool bwValid = true;
    bool fwValid = true;

    void addToBW(tStateID from, tLabelID alpha, tStateID to, bool &didIncrease);

    void addToFW(tStateID from, tLabelID alpha, tStateID to, bool didIncrease);

    // iterators

    // iterator over outgoing arcs of a state
    bool iterBlocked = true;
    innerFW::iterator iterToState;
    innerFW::iterator iterToStateEnd;

    set<tLabelID>::iterator iterLabel;
    set<tLabelID>::iterator iterLabelEnd;

public:
    TransitionContainer();

    ~TransitionContainer();

    // perform index transformation
    void *updateFAData(int *indexMapping);

    // get all states with an edge labeled "label" that reaches to
    set<tStateID> *getFrom(int to, int label);

    // add a rule to the transition system
    void addRule(tStateID from, tLabelID alpha, tStateID to);

    // iterate over outgoing edges
    void outIterInit(tStateID from);

    bool outIterNext(tLabelID *alpha, tStateID *to);

    // iterate over all transitions
    void fullIterInit();

    bool fullIterNext(tStateID *from, tLabelID *label, tStateID *to);

    // temporarily stored transition system
    void addTempArc(tStateID from, tLabelID label, tStateID to);

    // free memory
    void freeFW();

    void freeBW();

    void switchToTemp();

    void copyBWtoFW();
    void copyFWtoBW();
};


#endif //TOAD_TRANSITIONCONTAINER_H

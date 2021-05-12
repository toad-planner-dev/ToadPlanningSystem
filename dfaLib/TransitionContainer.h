//
// Created by dh on 19.03.21.
//

#ifndef TOAD_TRANSITIONCONTAINER_H
#define TOAD_TRANSITIONCONTAINER_H

#include <vector>
#include <set>
#include <unordered_map>
#include <climits>

using namespace std;

// typedef
typedef unsigned int tStateID;
typedef unsigned int tLabelID;
typedef unordered_map<tLabelID, set<tStateID> *> innerFW;
typedef unordered_map<tStateID, set<tStateID> *> innerBW;
typedef unordered_map<tStateID, innerFW *> transitionFW;
typedef unordered_map<tLabelID, innerBW *> transitionBW;
const int epsilon = USHRT_MAX;

class TransitionContainer {


    // - it is possible to either maintain both directions or just one
    // - by default, only the backward system is maintained (it might be copied to forward, *moved* to forward)
    // - the following variables show which one is valid, functions like "addRule" will only maintain those that are valid
    bool bwValid = false;
    bool fwValid = true;

    void addToBW(tStateID from, tLabelID alpha, tStateID to, bool &didIncrease);

    void addToFW(tStateID from, tLabelID alpha, tStateID to, bool didIncrease);

    // iterators

    // iterator over outgoing arcs of a state
    bool iterBlocked = true;
    innerFW::iterator iterLabel;
    innerFW::iterator iterLabelEnd;

    set<tStateID>::iterator iterToState;
    set<tStateID>::iterator iterToStateEnd;



    bool iterBlocked2 = true;

    transitionFW::iterator iterFrom;
    transitionFW::iterator iterFromEnd;

    innerFW::iterator iterLabel2;
    innerFW::iterator iterLabelEnd2;

    set<tStateID>::iterator iterTo2;
    set<tStateID>::iterator iterToEnd2;


public:
    int numTransitions = 0;

    transitionFW *forward = nullptr; // from -> (label -> to)
    transitionBW *backward = nullptr; // label -> (to -> from)

    TransitionContainer();

    ~TransitionContainer();

    // perform index transformation
    void *indexTransformation(int *indexMapping);

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

    // free memory
    void freeFW();

    void freeBW();

    void moveBWtoFW();
    void moveFWtoBW();

    void ensureBW();

    void ensureFW();

    void outIterInit(tStateID from, tLabelID  label);
    bool outIterNext(tStateID *to);

};


#endif //TOAD_TRANSITIONCONTAINER_H

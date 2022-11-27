//
// Created by dh on 19.08.22.
//

#include "HeuFaDist.h"
#include "../utils/IntPairHeap.h"

void HeuFaDist::writeHeuristicLookupTable(VectorFst<StdArc> *fa, bool addedGoalState) {

    cout << "Creating heuristic lookup table" << endl;
    int numStates = fa->NumStates();
    if (addedGoalState) {
        // there have been multiple goal states that have been compiled away
        // -> there is one more states, and one more action is needed
        numStates++;
    }
    set<int> orgFinal;

    StdVectorFst* fstRev = new StdVectorFst();
    fstRev->SetProperties(kAcceptor, true);

    for (int i = 0; i < numStates; i++) {
        fstRev->AddState();
    }
    for (StateIterator<StdVectorFst> siter(*fa); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        if (fa->Final(state_id) == 0) {
            orgFinal.insert(state_id);
        }
        for (ArcIterator<StdFst> aiter(*fa, state_id); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            fstRev->AddArc(arc.nextstate, StdArc(arc.ilabel + 1, arc.ilabel + 1, 0, state_id));
        }
    }

    int* hVals = new int[numStates];
    for (int i = 0; i < numStates; i++) {
        hVals[i] = INT_MAX;
    }
    int initVal = 0;
    if (addedGoalState) {
        initVal = 1; // from org final states, there is another epsilon step to go
        hVals[numStates - 1] = 0; // the last state is not reached in normal transition system
    }
    progression::IntPairHeap heap(1000);
    for(int sF : orgFinal) {
        heap.add(initVal, sF);
    }
    while (!heap.isEmpty()) {
        int costs = heap.topKey();
        int state = heap.topVal();
        heap.pop();
        if (hVals[state] <= costs) {
            continue;
        }
        hVals[state] = costs;
        for (ArcIterator<StdFst> aiter(*fstRev, state); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int newState = arc.nextstate;
            int arcCosts = 1;
//            if (arc.ilabel == 0) { // want hops, not h value -> epsilon cost also
//                arcCosts = 0;
//            }
            int newCosts = costs + arcCosts;
            if (newCosts < hVals[newState]) {
                heap.add(newCosts, newState);
            }
        }
    }

    ofstream hfile;
    hfile.open("dfad.heuristic");
    hfile << numStates << "\n";
    for (int i = 0; i < numStates; i++) {
        hfile << hVals[i] << "\n";
    }
    delete hVals;
    hfile.close();
    delete fstRev;
}

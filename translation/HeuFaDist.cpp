//
// Created by dh on 19.08.22.
//

#include "HeuFaDist.h"
#include "../utils/IntPairHeap.h"

void HeuFaDist::writeHeuristicLookupTable(VectorFst<StdArc> *fstInit) {

    cout << "Creating heuristic lookup table" << endl;
    const int numStates = fstInit->NumStates();
    set<int> final;
    StdVectorFst* fstRev = new StdVectorFst();
    fstRev->SetProperties(kAcceptor, true);

    for (int i = 0; i < numStates; i++) {
        fstRev->AddState();
    }
    for (StateIterator<StdVectorFst> siter(*fstInit); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        if (fstInit->Final(state_id) == 0) {
            final.insert(state_id);
        }
        for (ArcIterator<StdFst> aiter(*fstInit, state_id); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            fstRev->AddArc(arc.nextstate, StdArc(arc.ilabel + 1, arc.ilabel + 1, 0, state_id));
        }
    }

    int* hVals = new int[numStates];
    for (int i = 0; i < numStates; i++) {
        hVals[i] = INT_MAX;
    }
    progression::IntPairHeap heap(1000);
    for(int sF : final) {
        heap.add(0, sF);
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
            if (arc.ilabel == 0) {
                arcCosts = 0;
            }
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

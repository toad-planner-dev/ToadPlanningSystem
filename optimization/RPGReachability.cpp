//
// Created by dh on 16.10.20.
//

#include <cstring>
#include "RPGReachability.h"

RPGReachability::RPGReachability(progression::Model *htn) {
    this->m = htn;
    hValPropInit = new int[m->numStateBits];
    for (int i = 0; i < m->numStateBits; i++) {
        hValPropInit[i] = UNREACHABLE;
    }
    queue = new IntPairHeap(m->numStateBits * 2);
    numSatPrecs = new int[m->numActions];
    hValOp = new int[m->numActions];
    hValProp = new int[m->numStateBits];
    markedFs.init(m->numStateBits);
    markedOps.init(m->numActions);
    needToMark.init(m->numStateBits);
}

void RPGReachability::computeReachability(FiniteAutomaton *dfa) {
    memcpy(numSatPrecs, m->numPrecs, sizeof(int) * m->numActions);
    memcpy(hValOp, m->actionCosts, sizeof(int) * m->numActions);
    memcpy(hValProp, hValPropInit, sizeof(int) * m->numStateBits);

    queue->clear();
    for (int i = 0; i < m->s0Size; i++) {
        int f = m->s0List[i];
        queue->add(0, f);
        hValProp[f] = 0;
    }

    for (int i = 0; i < m->numPrecLessActions; i++) {
        int ac = m->precLessActions[i];
        for (int iAdd = 0; iAdd < m->numAdds[ac]; iAdd++) {
            int fAdd = m->addLists[ac][iAdd];
            hValProp[fAdd] = m->actionCosts[ac];
            queue->add(hValProp[fAdd], fAdd);
        }
    }
    while (!queue->isEmpty()) {
        int pVal = queue->topKey();
        int prop = queue->topVal();
        queue->pop();
        if (hValProp[prop] < pVal)
            continue;
        for (int iOp = 0; iOp < m->precToActionSize[prop]; iOp++) {
            int op = m->precToAction[prop][iOp];
            hValOp[op] += pVal;
            if (--numSatPrecs[op] == 0) {
                for (int iF = 0; iF < m->numAdds[op]; iF++) {
                    int f = m->addLists[op][iF];
                    if (hValOp[op] < hValProp[f]) {
                        hValProp[f] = hValOp[op];
                        queue->add(hValProp[f], f);
                    }
                }
            }
        }
    }
}

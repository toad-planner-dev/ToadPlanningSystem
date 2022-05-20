//
// Created by dh on 23.01.21.
//

#include "dfsDelRelReachability.h"

void dfsDelRelReachability::reachabilityAnalysis(FiniteAutomaton *dfa, progression::Model *htn) {
    this->dfa = dfa->fda2;
    this->htn = htn;
    set<int>* state = new set<int>;
    for(int i = 0; i < htn->s0Size; i++) {
        state->insert(htn->s0List[i]);
    }

    int dist = calc(0, state);
}

int dfsDelRelReachability::calc(int dfaState, set<int> *state) {
    int res = -1;
    unordered_map<int, set<int> *>* outgoing = dfa.at(dfaState);
    for(auto& edge : *outgoing) {
        int newState = edge.first;
        for(int a : *edge.second) {
            bool applicable = true;
            for (int i = 0; i < htn->numPrecs[a]; i++) {
                int prec = htn->precLists[a][i];
                if(state->find(prec) == state->end()) {
                    applicable = false;
                    break;
                }
            }
            if (applicable) {
                set<int> delta;
                for(int i = 0; i < htn->numAdds[a]; i++) {
                    int add = htn->addLists[a][i];
                    if(state->find(add) == state->end()) {
                        delta.insert(add);
                        state->insert(add);
                    }
                    // do something
                    int val = calc(newState, state);
                    if (val >= 0) {
                        int newRes = val + htn->actionCosts[a];
                        if ((res < 0) || (res > newRes)) {
                            res = newRes;
                        }
                    }

                    // clean up
                    for(int d : delta) {
                        state->erase(d);
                    }
                }
            }
        }
    }
    return false;
}

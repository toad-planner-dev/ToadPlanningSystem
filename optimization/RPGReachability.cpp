//
// Created by dh on 16.10.20.
//

#include <cstring>
#include "RPGReachability.h"

RPGReachability::RPGReachability(progression::Model *htn) {
    this->m = htn;
    hValPropInit = new int[m->numStateBits];
    numSatPrecs = new int[m->numActions];
}

void RPGReachability::computeReachability(FiniteAutomaton *dfa) {
    /*
    memcpy(numSatPrecs, m->numPrecs, sizeof(int) * m->numActions);

    dfa->actionMap = new unordered_map<int, set<pair<int, int>> *>;

    set<int>* dfaNow = new set<int>;
    set<int>* dfaNext = new set<int>;
    set<int>* fixpoint = new set<int>;
    dfaNow->insert(0);

    set<int>* state = new set<int>;
    set<int>* newFeatures = new set<int>;

    for(int i = 0; i < m->s0Size; i++) {
        newFeatures->insert(m->s0List[i]);
    }

    while(true) {
        if(newFeatures->empty()) {
            int lastSize = fixpoint->size();
            fixpoint->insert(dfaNow->begin(), dfaNow->end());
            if(lastSize == fixpoint->size()) {
                break;
            }
        } else { // new state features have been added
            fixpoint->clear();
        }

        // update state-based applicability
        for (int f : *newFeatures) {
            for (int i = 0; i < m->precToActionSize[f]; i++) {
                int a = m->precToAction[f][i];
                if(numSatPrecs[a] > 0)
                    numSatPrecs[a]--;
            }
        }
        state->insert(newFeatures->begin(), newFeatures->end());
        newFeatures->clear();

        // test applicability of all actions applicable from the current DFA states
        dfaNext->clear();
        for(int from : *dfaNow) {
            if (dfa->fda2.find(from) == dfa->fda2.end()) continue;
            for (auto &it2: *dfa->fda2.at(from)) {
                int to = it2.first;
                set<int> actions = *it2.second;
                for (int a : actions) {
                    if(a == -1) {
                        addToReachable(dfa->actionMap, a, from, to);
                        continue;
                    }
                    if (numSatPrecs[a] <= 0) { // state-based applicable
                        for (int iF = 0; iF < m->numAdds[a]; iF++) {
                            int f = m->addLists[a][iF];
                            if(state->find(f) == state->end()) {
                                newFeatures->insert(f);
                            }
                        }
                        dfaNext->insert(to);
                        addToReachable(dfa->actionMap, a, from, to);
                    }
                }
            }
        }
        if (dfaNext->empty()) {
            break;
        }
        set<int>* temp = dfaNow;
        dfaNow = dfaNext;
        dfaNext = temp;
        dfaNext->clear();
    }
    dfa->actionMapDone = true;
    int transitions = 0;
    for(int i = 0; i < m->numActions; i++) {
        if(dfa->actionMap->find(i) != dfa->actionMap->end()) {
            transitions += dfa->actionMap->at(i)->size();
        }
    }
    cout << "- new DFA contains " << transitions << " transitions." << endl;
    */
}

void RPGReachability::addToReachable(unordered_map<int, set<pair<int, int>> *> *actionMap, int a, int from, int to) {
    if (actionMap->find(a) == actionMap->end()) {
        set<pair<int, int>> *s = new set<pair<int, int>>;
        actionMap->insert({a, s});
    }
    actionMap->at(a)->insert({from, to});
}

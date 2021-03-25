//
// Created by dh on 22.01.21.
//

#include <fstream>
#include <cassert>
#include "hDfaDistance.h"
#include "utils/IntPairHeap.h"

void hDfaDistance::write(const string& filename, FiniteAutomaton *dfa, progression::Model *htn) {
    ofstream hfile;
    hfile.open(filename);
    cout << "- generate inverse graph" << endl;
    generateInverseGraph(dfa, htn);
    cout << "- calculate heuristics" << endl;
    generateHeuVals();
    cout << "- writing heuristic file" << endl;
    for (auto& it : *hVals) {
        hfile << it.first << " " << it.second << "\n";
    }
    delete hVals;
    hfile.close();
}

void hDfaDistance::generateHeuVals() {
    this->hVals = new unordered_map<int, int>;
    IntPairHeap heap(1000);
    heap.add(0,1);

    while (!heap.isEmpty()) {
        int costs = heap.topKey();
        int state = heap.topVal();
        heap.pop();
        if (hVals->find(state) != hVals->end()) {
            if (hVals->at(state) <= costs) {
                continue;
            }
        }
        hVals->insert({state, costs});

        if (g->find(state) != g->end()) {
            unordered_map<int, int> *s = g->at(state);
            for (auto& it : *s) {
                int newState = it.first;
                int newCosts = it.second + costs;
                heap.add(newCosts, newState);
            }
        }
    }
}

void hDfaDistance::generateInverseGraph(FiniteAutomaton *dfa, progression::Model *htn) {
    this->g = new unordered_map<int, unordered_map<int, int>*>;
    for (auto &it: dfa->fda2) {
        int from = it.first;
        for (auto &it2: *dfa->fda2.at(from)) {
            int to = it2.first;
            set<int> *labels = it2.second;
            int minVal = INT_MAX;
            for(int a : *labels) {
                minVal = min(htn->actionCosts[a], minVal);
            }
            if (g->find(to) == g->end()) {
                unordered_map<int, int> *s = new unordered_map<int, int>;
                g->insert({to, s});
            }
            if (g->at(to)->find(from) == g->at(to)->end()) {
                g->at(to)->insert({from, minVal});
            } else {
                g->at(to)->at(from) = minVal;
            }
        }
    }
}

//
// Created by dh on 28.09.20.
//

#include "FiniteAutomaton.h"
#include <iostream>
#include <vector>

void FiniteAutomaton::addRule(int q0, int alpha, int q1) {
    if ((alpha == -1) && (q0 == q1))
        return;
    set<int> *labels;
    if (fda2.find(q0) == fda2.end()) {
        fda2[q0] = new unordered_map<int, set<int>*>;
    }

    if (fda2[q0]->find(q1) == fda2[q0]->end()) {
        labels = new set<int>;
        fda2[q0]->insert({q1, labels});
    } else {
        labels = fda2[q0]->at(q1);
    }
    if (labels->find(alpha) == labels->end())
        this->numTransitions++;
    labels->insert(alpha);
}

/*
void FiniteAutomaton::print(string *pVector, int start, int final) {
    cout << "digraph G {" << endl;
    for (auto &it: fda) {
        Pair *p = it.first;
        set<int> *labels = it.second;
        cout << "    n" << p->from << " -> n" << p->to << " [label=\"";
        bool first = true;
        for (int l : *labels) {
            if (first){
                first = false;
            } else {
                cout << " OR ";
            }
            if (l == -1) {
                cout << "Epsilon";
            } else {
                cout << pVector[l];
            }
        }
        cout << "\"];" << endl;
    }
    cout << "}" << endl;
}
*/

unordered_map<int, unordered_map<int, set<int> *> *> * FiniteAutomaton::getActionMap() {
    if(!actionMapDone) {
        this->actionMap = new unordered_map<int, unordered_map<int, set<int>*>*>;
        for (auto &it: fda2) {
            int from = it.first;
            for (auto &it2: *fda2.at(from)) {
                int to = it2.first;
                set<int> *labels = it2.second;

                for (int l : *labels) {
                    if (actionMap->find(l) == actionMap->end()) {
                        unordered_map<int, set<int>*>* s = new unordered_map<int, set<int>*>;
                        actionMap->insert({l, s});
                    }
                    if(actionMap->at(l)->find(from) == actionMap->at(l)->end()) {
                        actionMap->at(l)->insert({from,  new set<int>});
                    }
                    actionMap->at(l)->at(from)->insert(to);
                }
            }
        }
        actionMapDone = true;
    }
    return this->actionMap;
}

//
// Created by dh on 28.09.20.
//

#include "FiniteAutomaton.h"
#include <iostream>
#include <vector>

void FiniteAutomaton::addRule(int q0, int alpha, int q1) {
    Pair *p = new Pair(q0, q1);
    set<int> *labels;
    if (this->fda.find(p) == fda.end()) {
        labels = new set<int>;
        this->fda[p] = labels;
    } else {
        labels = this->fda[p];
    }
    if(labels->find(alpha) == labels->end())
        this->numTransitions++;
    labels->insert(alpha);
}

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

unordered_map<int, set<pair<int, int> *> *>* FiniteAutomaton::getActionMap() {
    if(!actionMapDone) {
        this->actionMap = new unordered_map<int, set<pair<int, int> *> *>;
        for (auto &it: this->fda) {
            Pair *p = it.first;
            set<int> *labels = it.second;
            for (int l : *labels) {
                if (actionMap->find(l) == actionMap->end()) {
                    set<pair<int, int> *> *s = new set<pair<int, int> *>;
                    actionMap->insert({l, s});
                }
                actionMap->at(l)->insert(new pair<int, int>(p->from, p->to));
            }
        }
        actionMapDone = true;
    }
    return this->actionMap;
}

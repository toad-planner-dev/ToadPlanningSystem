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
    labels->insert(alpha);
}

void FiniteAutomaton::print(string *pVector, int start, int final) {
    cout << "digraph G {" << endl;
    for (auto &it: fda) {
        Pair *p = it.first;
        set<int> *labels = it.second;
        cout << "    n" << p->from << " -> n" << p->to << " [label=\"";
        for (int l : *labels) {
            if (l == -1) {
                cout << "Epsilon" << " ";
            } else {
                cout << pVector[l] << " OR ";
            }
        }
        cout << "\"];" << endl;
    }
    cout << "}" << endl;
}

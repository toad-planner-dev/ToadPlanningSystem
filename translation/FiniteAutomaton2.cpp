//
// Created by dh on 28.09.20.
//

#include "FiniteAutomaton.h"
#include <iostream>
#include <vector>

void FiniteAutomaton::addRule(int q0, int alpha, int q1) {
    if (alpha == -1) {
        if (q0 == q1) {
            return;
        }
    }
    fda->add(q0, alpha, q1);
}

/*
void FiniteAutomaton::print(string *pVector, int start, int final) {
    unordered_map<Pair*, set<int>*, pairHash, pairComp> g;
    bool showlabels = true;
    cout << "digraph G {" << endl;
    for (auto &it: fda2) {
        int from = it.first;
        for (auto &it2: *it.second) {
            int to = it2.first;
            set<int> *labels = it2.second;
            cout << "    n" << from << " -> n" << to;
            cout << " [label=\"";

            bool first = true;
            for (int l : *labels) {
                if (first) {
                    first = false;
                } else {
                    cout << "; ";
                }
                if (showlabels) {
                    if (l == -1) {
                        cout << "Epsilon";
                    } else {
                        cout << pVector[l];
                    }
                } else {
                    cout << l;
                }
            }

            cout << "\"];" << endl;
        }
    }
    cout << "}" << endl;
}
*/
unordered_map<int, unordered_map<int, set<int> *> *> * FiniteAutomaton::getActionMap() {
  /*
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
*/
}


FiniteAutomaton::~FiniteAutomaton() {
/*
    #ifndef NDEBUG


    for (pair<int, unordered_map<int, set<int>*>*> i=fda2.begin(); i != fda2.end(); i++)
    fda2.begin().
    for(x = fda2.begin()) {
        for(auto y : fda2) {
            delete y.second;
        }
        delete x.second;
    }
#endif*/
}

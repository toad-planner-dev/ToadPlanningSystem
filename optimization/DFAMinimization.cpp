//
// Created by dh on 12.11.20.
//

#include "DFAMinimization.h"
#include <map>
#include <cassert>
#include <algorithm>

void DFAMinimization::minimize(Model *htn, FiniteAutomaton *dfa_org, int S0, int F) {
    unordered_map<int, vector<pair<int, set<int> *>> *> *dfa = new unordered_map<int, vector<pair<int, set<int> *>> *>;
    // a -> [to, {from1, from2, ...}]
    int numStates = dfa_org->stateID;
    cout << "- building inverse model" << endl;

    for (auto &it: dfa_org->fda2) {
        int from = it.first;
        for (auto &it2: *dfa_org->fda2.at(from)) {
            int to = it2.first;
            set<int> *labels = it2.second;

            for (int l : *labels) {
                if (dfa->find(l) == dfa->end()) {
                    vector<pair<int, set<int> *>> *s = new vector<pair<int, set<int> *>>;
                    dfa->insert({l, s});
                }
                vector<pair<int, set<int> *>> *s = dfa->at(l);
                int iTo = indexOf(s, to);
                if (iTo == -1) {
                    insert(s, to);
                    iTo = indexOf(s, to);
                    assert (iTo >= 0);
                }
                s->at(iTo).second->insert(from);
            }
        }
    }

    vector<pair<int, set<int> *>> *P = new vector<pair<int, set<int> *>>;
    vector<pair<int, set<int> *>> *W = new vector<pair<int, set<int> *>>;
    set<int> *init = new set<int>;
    set<int> *final = new set<int>;
    for (int i = 0; i < dfa_org->stateID; i++) {
        init->insert(i);
    }
    init->erase(F);
    final->insert(F);

    P->push_back({0, init});
    P->push_back({1, final});
    W->push_back({0, init});
    W->push_back({1, final});
    int setID = 2;
    cout << "- Generating new states" << endl;
    //cout << "W-Size: " << W->size();
    while (!W->empty()) {
        int id = W->back().first;
        set<int> *A = W->back().second;
        W->pop_back();

        for (int c = 0; c < htn->numActions; c++) {
            if(dfa->find(c) == dfa->end()) {
                continue;
            }
            vector<pair<int, set<int> *>> * rules = dfa->at(c);
            set<int>* X = new set<int>;
            for (int i = 0; i < rules->size(); i++) {
                int to = rules->at(i).first;
                if (A->find(to) != A->end()) {
                    X->insert(rules->at(i).second->begin(), rules->at(i).second->end()); // these are the from values
                }
            }
            int oldSize = P->size();
            for (int i = 0; i < oldSize; i++) {
                set<int>* Y = P->at(i).second;

                if(Y->size() <= X->size()) { // Y subset of X?
                    bool isSubset = true;
                    for(int y : *Y) {
                        if(X->find(y) == Y->end()) {
                            isSubset = false;
                            break;
                        }
                    }
                    if(isSubset) {
                        continue;
                    }
                }

                set<int>* inter = new set<int>;
                int size = Y->size();
                for (int e : *X) {
                    Y->erase(e);
                    if(size != Y->size()) {
                        inter->insert(e);
                        size = Y->size();
                    }
                }
                if (!inter->empty()) {
                    pair<int, set<int> *> pInter = {setID++, inter};
                    P->push_back(pInter);

                    int iW = indexOf(W, id);
                    if (iW != -1) { // Y also contained in W
                        W->push_back(pInter);
                    } else {
                        if(inter->size() < Y->size()) {
                            W->push_back(pInter);
                        } else {
                            W->push_back(P->at(i));
                        }
                    }
                    sort(W, 0, W->size()-1);
                } else {
                    delete inter;
                }
            }
            sort(P, 0, P->size()-1);
        }
        //cout << " -> " << W->size();
    }
    cout << " DONE!" << endl;

    cout << "- states: " << dfa_org->stateID << " to " << P->size() << endl;
    /*
    for(int i = 0; i < P->size(); i++) {
        pair<int, set<int> *> Pi = P->at(i);
        cout << i << " = {";
        for(int s : *Pi.second) {
            cout << s << " ";
        }
        cout << "}" << endl;
    }*/

    delete W;
    for(pair<int, set<int> *> p : *P) {
        delete p.second;
    }
    delete P;
    delete dfa;
}

int DFAMinimization::indexOf(vector<pair<int, set<int> *>> *v, int elem) {
    int left = 0;
    int right = v->size() - 1;
    while (left <= right) {
        int middle = left + ((right - left) / 2);
        if (v->at(middle).first == elem) {
            return middle;
        } else if (v->at(middle).first > elem) {
            right = middle - 1;
        } else {
            left = middle + 1;
        }
    }
    return -1;
}

void DFAMinimization::insert(vector<pair<int, set<int> *>> *v, int elem) {
    v->push_back({elem, new set<int>});
    sort(v, 0, v->size() - 1);
}

void DFAMinimization::sort(vector<pair<int, set<int> *>> *v, int l, int r) {
    if (l < r) {
        int p = part(v, l, r);
        sort(v, l, p);
        sort(v, p + 1, r);
    }
}

int DFAMinimization::part(vector<pair<int, set<int> *>> *v, int l, int r) {
    int pivot = v->at((l + r) / 2).first;
    int i = l - 1;
    int j = r + 1;
    while (true) {
        do {
            i++;
        } while (v->at(i).first < pivot);
        do {
            j--;
        } while (v->at(j).first > pivot);
        if (i >= j) {
            return j;
        }
        pair<int, set<int> *> temp = v->at(i);
        v->at(i) = v->at(j);
        v->at(j) = temp;
    }
}

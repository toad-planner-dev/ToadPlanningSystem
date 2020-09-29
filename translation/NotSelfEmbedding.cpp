//
// Created by dh on 28.09.20.
//

#include "NotSelfEmbedding.h"
#include <cassert>

void NotSelfEmbedding::addRule(vector<int> *rule) {
    rules.push_back(rule);
}

void NotSelfEmbedding::makeFA(int q0, vector<int> *alpha, int q1) {
    if (alpha->size() == 1) {
        makeFA(q0, alpha->at(0), q1);
    } else if (alpha->size() > 1) {
        int q = this->stateID++;
        int X = alpha->at(0);
        alpha->erase(alpha->begin());
        makeFA(q0, X, q);
        makeFA(q, alpha, q1);
    }
}

void NotSelfEmbedding::makeFA(int q0, int alpha, int q1) {
    if (alpha == Epsilon) {
        fa.addRule(q0, alpha, q1);
    } else if (isTerminalSym(alpha)) {
        fa.addRule(q0, alpha, q1);
    } else {
        int A = alpha;
        if (Ni->at(A) >= 0) {
            // get partition containing A
            int i = Ni->at(A);
            vector<int> *NiS = Nisets.at(i);

            // create new states
            vector<int> qB;
            for (int j = 0; j < NiS->size(); j++) {
                qB.push_back(this->stateID++);
            }

            // left recursion
            if (this->recursion->at(i) == left) {
                // find rules that decompose some C belonging to the same partition
                for (std::vector<vector<int> *>::iterator it = rules.begin(); it != rules.end(); ++it) {
                    vector<int> *rule = *it;
                    int C = rule->at(0); // left-hand side
                    int j = Ni->at(C);
                    int D = rule->at(1); // first right-hand side
                    int k = Ni->at(D);
                    bool addEpsilon = false;
                    if ((j == i) && (k != i)) {
                        int qC = getNewState(NiS, qB, C);
                        makeFA(q0, copySubSeq(rule, 1, rule->size()), qC);
                        addEpsilon = true;
                    } else if ((j == i) && (k == i)) {
                        int qD = getNewState(NiS, qB, D);
                        int qC = getNewState(NiS, qB, C);
                        makeFA(qD, copySubSeq(rule, 2, rule->size()), qC);
                        addEpsilon = true;
                    }
                    if (addEpsilon) {
                        int qA = getNewState(NiS, qB, A);
                        makeFA(qA, Epsilon, q1);
                    }
                }
            }
        }
    }
}

int NotSelfEmbedding::getNewState(vector<int> *NiS, vector<int> &qB, int C) const {
    for (int i = 0; i < NiS->size(); i++) {
        if (NiS->at(i) == C) {
            return qB.at(i);
        }
    }
    assert(false);
    return -1;
}

int NotSelfEmbedding::isTerminalSym(int a) {
    return (a != Epsilon) && (a < this->numPrim);
}

vector<int> *NotSelfEmbedding::copySubSeq(vector<int> *in, int from, int to) {
    vector<int> *out = new vector<int>;
    assert(from > 0);
    assert(from < to);
    assert(from < in->size());
    assert(to > 0);
    assert(to <= in->size());
    for (int i = from; i < to; i++) {
        out->push_back(in->at(i));
    }
    return out;
}

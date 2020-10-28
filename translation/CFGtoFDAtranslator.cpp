//
// Created by dh on 28.09.20.
//

#include "CFGtoFDAtranslator.h"
#include "../ModelWriter.h"
#include <cassert>
#include <unordered_map>
#include <fstream>

void CFGtoFDAtranslator::addRule(vector<int> *r) {
    grRule *r2 = new grRule();
    r2->left = r->at(0);
    r2->rLength = r->size() - 1;
    r2->right = new int[r2->rLength];
    for (int i = 1; i < r->size(); i++) {
        r2->right[i - 1] = r->at(i);
    }

    //printRule(r2);
    tempRules.push_back(r2);
}

void CFGtoFDAtranslator::makeFA(int q0, vector<int> *alpha, int q1) {
    assert(alpha->size() >= 0);
    if (alpha->size() == 1) {
        makeFA(q0, alpha->at(0), q1);
    } else if (alpha->size() > 1) {
        int q = this->dfa->stateID++;
        int X = alpha->at(0);
        vector<int> *beta = new vector<int>;
        for (int i = 1; i < alpha->size(); i++)
            beta->push_back(alpha->at(i));
        //alpha->erase(alpha->begin());
        makeFA(q0, X, q);
        makeFA(q, beta, q1);
    }
}

void CFGtoFDAtranslator::makeFA(int q0, int A, int q1) {
    if (A == Epsilon) {
        dfa->addRule(q0, A, q1);
    } else if (isTerminalSym(A)) {
        dfa->addRule(q0, A, q1);
    } else {
        if (SymToNi[A] >= 0) {
            // get partition containing A
            int Nl = SymToNi[A];

            // create new states
            qB.clear();
            for (int j = 0; j < NiSize[Nl]; j++) {
                int task = Ni[Nl][j];
                if (task != A) {
                    qB[task] = this->dfa->stateID++;
                }
            }

            // left recursion
            if (NiRec[Nl] == recLeft) {
                qB[A] = q1;
                // iterate rules that decompose some C belonging to the same partition
                for (int i = 0; i < NiSize[Nl]; i++) {
                    int C = Ni[Nl][i]; // left-hand side
                    int qC = qB[C];
                    for (int l = rFirst[C]; l <= rLast[C]; l++) {
                        grRule *rule = rules[l];
                        //printRule(rule);
                        int D = rule->right[0]; // first right-hand side
                        int Nk = SymToNi[D];
                        if (Nk != Nl) {
                            makeFA(q0, copySubSeq(rule, 0, rule->rLength), qC);
                        } else {
                            int qD = qB[D];
                            makeFA(qD, copySubSeq(rule, 1, rule->rLength), qC);
                        }
                    }
                }
                int qA = qB[A];
                dfa->addRule(qA, Epsilon, q1);
            } else { // right or cyclic
                qB[A] = q0;
                // iterate rules that decompose some C belonging to the same partition
                for (int i = 0; i < NiSize[Nl]; i++) {
                    int C = Ni[Nl][i]; // left-hand side
                    int qC = qB[C];
                    for (int l = rFirst[C]; l <= rLast[C]; l++) {
                        grRule *rule = rules[l];
                        //printRule(rule);
                        int D = rule->right[rule->rLength - 1]; // last right-hand side
                        int Nk = SymToNi[D];
                        if (Nk != Nl) {
                            makeFA(qC, copySubSeq(rule, 0, rule->rLength), q1);
                        } else {
                            int qD = qB[D];
                            makeFA(qC, copySubSeq(rule, 0, rule->rLength - 1), qD);
                        }
                    }
                }
                int qA = qB[A];
                dfa->addRule(q0, Epsilon, qA);
            }
        } else { // a non-recursive non-terminal
            for (int l = rFirst[A]; l <= rLast[A]; l++) {
                grRule *rule = rules[l];
                //printRule(rule);
                makeFA(q0, copySubSeq(rule, 0, rule->rLength), q1);
            }
        }
    }
}

/*
int CFGtoFDAtranslator::getNewState(int *NiS, int size, int *qB, int C) const {
    for (int i = 0; i < size; i++) {
        if (NiS[i] == C) {
            return qB[i];
        }
    }
    assert(false);
    return -1;
}
*/
int CFGtoFDAtranslator::isTerminalSym(int a) {
    return (a != Epsilon) && (a < this->numTerminals);
}

vector<int> *CFGtoFDAtranslator::copySubSeq(grRule *in, int from, int to) {
    vector<int> *out = new vector<int>;
    assert(from >= 0);
    assert(from < to);
    assert(from < in->rLength);
    assert(to > 0);
    assert(to <= in->rLength);
    for (int i = from; i < to; i++) {
        out->push_back(in->right[i]);
    }
    return out;
}

void CFGtoFDAtranslator::sortRules() {
    quick(0, numRules - 1);
}

void CFGtoFDAtranslator::quick(int l, int r) {
    if (l < r) {
        int t = divide(l, r);
        quick(l, t - 1);
        quick(t + 1, r);
    }
}

int CFGtoFDAtranslator::divide(int l, int r) {
    int i = l;
    int j = r - 1;
    grRule *pivot = rules[r];
    while (i < j) {
        while ((i < r) && (compareRules(rules[i], pivot) < 0)) {
            i++;
        }
        while ((j > l) && (compareRules(rules[j], pivot) >= 0)) {
            j--;
        }
        if (i < j) {
            grRule *temp = rules[i];
            rules[i] = rules[j];
            rules[j] = temp;
        }
    }
    if (compareRules(rules[i], pivot) > 0) {
        grRule *temp = rules[i];
        rules[i] = rules[r];
        rules[r] = temp;
    }
    return i;
}


void CFGtoFDAtranslator::analyseRules() {
    // copy rules
    this->numRules = tempRules.size();
    rules = new grRule *[numRules];
    for (int i = 0; i < tempRules.size(); i++) {
        rules[i] = tempRules.at(i);
    }

    cout << "- sorting rules...";
    sortRules();
    cout << "(done)" << endl;

    cout << "- analysing recursion...";
    // store rules for each symbol
    rFirst = new int[numSymbols];
    rLast = new int[numSymbols];
    for (int i = 0; i < numSymbols; i++) {
        rFirst[i] = -1;
        rLast[i] = -1;
    }
    for (int i = 0; i < numRules; i++) {
        grRule *r = rules[i];
        int symbol = r->left;
        if (rFirst[symbol] == -1) {
            rFirst[symbol] = i;
        }
        if (i > 0) {
            if (rules[i - 1]->left != rules[i]->left) {
                int sym = rules[i - 1]->left;
                rLast[sym] = i - 1;
            }
        }
    }
    rLast[rules[numRules - 1]->left] = numRules - 1;

    // check recursion structure of single rules
    for (int i = 0; i < numRules; i++) {
        this->determineRuleRecursion(rules[i]);
    }

    // check recursion of Ni sets
    NiRec = new int[NumNis];
    for (int i = 0; i < NumNis; i++) {
        NiRec[i] = recCycle;
    }
    for (int ni = 0; ni < NumNis; ni++) {
        for (int i = 0; i < NiSize[ni]; i++) {
            int symbol = this->Ni[ni][i];
            for (int j = rFirst[symbol]; j <= rLast[symbol]; j++) {
                grRule *r = rules[j];
                if (r->isLeftGenerating) {
                    if (NiRec[ni] == recLeft) {
                        NiRec[ni] = recSelf;
                    } else if (NiRec[ni] == recCycle) {
                        NiRec[ni] = recRight;
                    }
                }
                if (r->isRightGenerating) {
                    if (NiRec[ni] == recRight) {
                        NiRec[ni] = recSelf;
                    } else if (NiRec[ni] == recCycle) {
                        NiRec[ni] = recLeft;
                    }
                }
            }
        }
    }
    tempRules.clear();

    // create temp variable
    int maxSize = 0;
    for (int i = 0; i < NumNis; i++) {
        maxSize = max(maxSize, NiSize[i]);
    }
    cout << "(done)" << endl;

    if (printDebug) {
        for (int i = 0; i < numRules; i++) {
            printRule(rules[i]);
        }

        for (int i = numTerminals; i < numSymbols; i++) {
            cout << i << ": " << rFirst[i] << "-" << rLast[i] << endl;
        }
    }

    cout << "- partitions of recursive tasks:" << endl;
    bool containsRR = false;
    bool containsLR = false;
    bool containsSelf = false;
    bool containsCyclic = false;
    bool isRegular = true;
    for (int i = 0; i < NumNis; i++) {
        //cout << "  - N" << i << " ";
        if (NiRec[i] == recRight) {
            //cout << "right recursive";
            containsRR = true;
        } else if (NiRec[i] == recLeft) {
            //cout << "left recursive";
            containsLR = true;
        } else if (NiRec[i] == recSelf) {
            //cout << "self recursive";
            containsSelf = true;
            isRegular = false;
        } else if (NiRec[i] == recCycle) {
            //cout << "cyclic";
            containsCyclic = true;
        } else {
            //cout << "???";
        }
        for (int j = 0; j < NiSize[i]; j++) {
            //cout << " " << Ni[i][j];
        }
        //cout << endl;
    }

    cout << "- recursive structure:";
    if (containsRR) {
        cout << " [rrec=yes]";
    } else {
        cout << " [rrec=no]";
    }
    if (containsLR) {
        cout << " [lrec=yes]";
    } else {
        cout << " [lrec=no]";
    }
    if (containsSelf) {
        cout << " [srec=yes]";
    } else {
        cout << " [srec=no]";
    }
    if (containsCyclic) {
        cout << " [crec=yes]";
    } else {
        cout << " [crec=no]";
    }
    cout << endl;

    cout << "- instance properties:" << endl;
    if (NumNis == 0) {
        cout << "  - the instance is acyclic. [rec=acyclic]" << endl;
        cout << "  - using exact translation." << endl;
        this->isRegurlar = true;
    } else if (isRegular) {
        cout << "  - the instance is recursive, but not self-embedding, i.e. it is regular. [rec=nonSelfEmbedding]"
             << endl;
        cout << "  - using exact translation. [alg=exact]" << endl;
        this->isRegurlar = true;
    } else {
        cout
                << "  - the instance is recursive and self-embedding, i.e. it could not be shown that it is regular. [rec=selfEmbedding]"
                << endl;
        cout << "  - using approximate translation. [alg=approximate]" << endl;
        this->isRegurlar = false;
    }
}

int CFGtoFDAtranslator::compareRules(grRule *a, grRule *b) {
    if (a->left != b->left)
        return a->left - b->left;
    int smaller = std::min(a->rLength, b->rLength);
    for (int i = 0; i < smaller; i++)
        if (a->right[i] != b->right[i])
            return a->right[i] - b->right[i];
    if (a->rLength != b->rLength)
        return a->rLength - b->rLength;
    return 0;
}

void CFGtoFDAtranslator::determineRuleRecursion(grRule *r) {
    int Ni = this->SymToNi[r->left];
    if (Ni < 0) { // non-recursive
        return;
    }
    for (int j = 0; j < r->rLength; j++) {
        int rSym = r->right[j];
        int Nj = this->SymToNi[rSym];
        if (Ni == Nj) {
            if (j > 0) {
                r->isLeftGenerating = true;
            }
            if (j < (r->rLength - 1)) { // not the last index
                r->isRightGenerating = true;
            }
        }
    }
}

void CFGtoFDAtranslator::printRule(grRule *rule) {
    cout << "rule " << rule->left << " -> ";
    for (int i = 0; i < rule->rLength; i++) {
        if (i > 0) {
            cout << ", ";
        }
        cout << rule->right[i];
    }
    if (rule->isRightGenerating || rule->isLeftGenerating) {
        if (rule->isRightGenerating && rule->isLeftGenerating) {
            cout << " (RightGen, LeftGen)";
        } else if (rule->isRightGenerating) {
            cout << " (RightGen)";
        } else if (rule->isLeftGenerating) {
            cout << " (LeftGen)";
        }
    }
    cout << endl;
}

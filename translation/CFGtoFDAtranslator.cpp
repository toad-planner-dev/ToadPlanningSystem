//
// Created by dh on 28.09.20.
//

#include "CFGtoFDAtranslator.h"
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <map>
#include <vector>
#include "../dfaLib/FA.h"

void CFGtoFDAtranslator::addRule(vector<int> *r) {
    grRule *r2 = new grRule();
    r2->left = r->at(0);
    r2->rLength = r->size() - 1;
    r2->right = new int[r2->rLength];
    for (int i = 1; i < r->size(); i++) {
        r2->right[i - 1] = r->at(i);
    }
    tempRules.push_back(r2);
}

void CFGtoFDAtranslator::makeFA(int q0, vector<int> *alpha, int q1) {
    assert(alpha->size() > 0);
    if (alpha->size() == 1) {
        makeFA(q0, alpha->at(0), q1);
    } else if (alpha->size() > 1) {
        int q = this->dfa->stateID++;
        int X = alpha->at(0);
        vector<int> *beta = new vector<int>;
        for (int i = 1; i < alpha->size(); i++)
            beta->push_back(alpha->at(i));
        makeFA(q0, X, q);
        makeFA(q, beta, q1);
    }
    delete alpha;
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
            unordered_map<int, int> qB;
            for (int j = 0; j < NiSize[Nl]; j++) {
                int task = Ni[Nl][j];
                int id = this->dfa->stateID++;
                qB[task] = id;
            }

            // left recursion
            if (NiRec[Nl] == recLeft) {
                // iterate rules that decompose some C belonging to the same partition
                for (int i = 0; i < NiSize[Nl]; i++) {
                    int C = Ni[Nl][i]; // left-hand side
                    int qC = qB[C];
                    for (int l = rFirst[C]; l <= rLast[C]; l++) {
                        grRule *rule = rules[l];
                        int D = rule->right[0]; // first right-hand side
                        int Nk = -1;
                        if (D >= 0) {
                            Nk = SymToNi[D];
                        }
                        if (Nk != Nl) {
                            makeFA(q0, copySubSeq(rule, 0, rule->rLength), qC);
                        } else {
                            int qD = qB[D];
                            if (rule->rLength > 1) {
                                makeFA(qD, copySubSeq(rule, 1, rule->rLength), qC);
                            } else {
                                makeFA(qD, -1, qC);
                            }
                        }
                    }
                }
                int qA = qB[A];
                dfa->addRule(qA, Epsilon, q1);
            } else { // right or cyclic
                // iterate rules that decompose some C belonging to the same partition
                for (int i = 0; i < NiSize[Nl]; i++) {
                    int C = Ni[Nl][i]; // left-hand side
                    int qC = qB[C];

                    for (int l = rFirst[C]; l <= rLast[C]; l++) {
                        grRule *rule = rules[l];
                        int D = rule->right[rule->rLength - 1]; // last right-hand side
                        int Nk = -1;
                        if (D >= 0) {
                            Nk = SymToNi[D];
                        }
                        if (Nk != Nl) {
                            makeFA(qC, copySubSeq(rule, 0, rule->rLength), q1);
                        } else {
                            int qD = qB[D];
                            if (rule->rLength > 1) {
                                makeFA(qC, copySubSeq(rule, 0, rule->rLength - 1), qD);
                            } else {
                                makeFA(qC, -1, qD);
                            }
                        }
                    }
                }
                int qA = qB[A];
                dfa->addRule(q0, Epsilon, qA);
            }
        } else { // a non-recursive non-terminal
            for (int l = rFirst[A]; l <= rLast[A]; l++) {
                grRule *rule = rules[l];
                makeFA(q0, copySubSeq(rule, 0, rule->rLength), q1);
            }
        }
    }
}

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
    //printRules();
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

void CFGtoFDAtranslator::initDataStructures() {
    cout << "- starting grammar simplification" << endl;

    // symbol to rules that decompose it
    map<int, vector<grRule *> *> *rulesDecomposing = new map<int, vector<grRule *> *>;

    // symbol to rules it is contained in the right side
    map<int, vector<grRule *> *> *containedInRight = new map<int, vector<grRule *> *>;

    // symbols that can be decomposed into an epsilon
    set<int> symWithEpsilonRules;

    // collect data
    set<int> onlyOnce;
    for (int i = 0; i < tempRules.size(); i++) {
        grRule *r = tempRules[i];
        r->rSymbolsSet = 0; // how many distinct symbols are on the right hand side
        if (r->rLength == 0) {
            symWithEpsilonRules.insert(r->left);
        }

        int sym = r->left;
        if (rulesDecomposing->find(sym) == rulesDecomposing->end()) {
            rulesDecomposing->insert({sym, new vector<grRule *>});
        }
        rulesDecomposing->at(sym)->push_back(r);

        onlyOnce.clear();
        for (int j = 0; j < r->rLength; j++) {
            sym = r->right[j];
            if (onlyOnce.find(sym) != onlyOnce.end()) {
                continue;
            }
            r->rSymbolsSet++;
            if (containedInRight->find(sym) == containedInRight->end()) {
                containedInRight->insert({sym, new vector<grRule *>});
            }
            containedInRight->at(sym)->push_back(r);
        }
    }

    // computing observability
    for (int i = 0; i < tempRules.size(); i++) {
        tempRules[i]->temp = tempRules[i]->rSymbolsSet;
    }
    vector<int> todo;
    vector<bool> reached;
    for (int i = 0; i < numSymbols; i++) {
        reached.push_back(false);
    }
    for (int i = 0; i < numTerminals; i++) {
        todo.push_back(i);
        reached[i] = true;
    }
    for (int s : symWithEpsilonRules) {
        todo.push_back(s);
        reached[s] = true;
    }
    while (!todo.empty()) {
        int s = todo.back();
        todo.pop_back();
        if (containedInRight->find(s) == containedInRight->end()) {
            continue;
        }
        for (grRule *r : *containedInRight->at(s)) {
            if (--r->temp == 0) {
                int sym = r->left;
                if (!reached[sym]) {
                    todo.push_back(r->left);
                    reached[sym] = true;
                }
            }
        }
    }
    vector<int> unobsSymbols;
    set<grRule *> deleteRules;
    for (int i = 0; i < numSymbols; i++) {
        if (!reached[i]) {
            unobsSymbols.push_back(i);
            for (grRule *r : *containedInRight->at(i)) {
                deleteRules.insert(r);
                r->markedForDelete = true;
            }
        }
    }
    cout << "- " << deleteRules.size() << " rules are not observable" << endl;

    // copy rules
    this->numRules = tempRules.size() - deleteRules.size();
    rules = new grRule *[numRules];
    int newI = 0;
    for (int i = 0; i < tempRules.size(); i++) {
        grRule *r = tempRules.at(i);
        if (r->markedForDelete)
            continue;
        if (r->rLength == 0) {
            r->rLength = 1;
            r->right = new int[1];
            r->right[0] = -1;
        }
        rules[newI++] = r;
    }

    cout << "- sorting rules...";
    sortRules();
    cout << "(done)" << endl;

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
}

void CFGtoFDAtranslator::analyseRules(bool writeProtocol) {
    cout << "- analysing recursion...";
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
        if (NiRec[i] == recRight) {
            containsRR = true;
        } else if (NiRec[i] == recLeft) {
            containsLR = true;
        } else if (NiRec[i] == recSelf) {
            containsSelf = true;
            isRegular = false;
        } else if (NiRec[i] == recCycle) {
            containsCyclic = true;
        }
    }
    if (printDebug) {
        for (int i = 0; i < NumNis; i++) {
            cout << "  - N" << i << " ";
            if (NiRec[i] == recRight) {
                cout << "right recursive";
            } else if (NiRec[i] == recLeft) {
                cout << "left recursive";
            } else if (NiRec[i] == recSelf) {
                cout << "self recursive";
            } else if (NiRec[i] == recCycle) {
                cout << "cyclic";
            } else {
                cout << "???";
            }

            for (int j = 0; j < NiSize[i]; j++) {
                cout << " " << Ni[i][j];
            }
            cout << endl;
        }
    }

    if (writeProtocol)
        cout << "- recursive structure:";
    if (writeProtocol) cout << " [";
    if (containsRR) {
        cout << "rrec=yes";
    } else {
        cout << "rrec=no";
    }
    if (writeProtocol) cout << "] [";
    else cout << " ";
    if (containsLR) {
        cout << "lrec=yes";
    } else {
        cout << "lrec=no";
    }
    if (writeProtocol) cout << "] [";
    else cout << " ";
    if (containsSelf) {
        cout << "srec=yes";
    } else {
        cout << "srec=no";
    }
    if (writeProtocol) cout << "] [";
    else cout << " ";
    if (containsCyclic) {
        cout << "crec=yes";
    } else {
        cout << "crec=no";
    }
    if (writeProtocol) cout << "]";
    cout << endl;

    cout << "- instance properties:" << endl;
    if (NumNis == 0) {
        cout << "  - the instance is acyclic.";
        if (writeProtocol) cout << " [rec=acyclic]";
        cout << endl;
        cout << "  - using exact translation." << endl;
        this->isRegurlar = true;
    } else if (isRegular) {
        cout << "  - the instance is recursive, but not self-embedding, i.e. it is regular.";
        if (writeProtocol) cout << " [rec=nonSelfEmbedding]";
        cout << endl;
        cout << "  - using exact translation.";
        if (writeProtocol) cout << " [alg=exact]";
        cout << endl;
        this->isRegurlar = true;
    } else {
        cout << "  - the instance is recursive and self-embedding, i.e. it could not be shown that it is regular.";
        if (writeProtocol) cout << " [rec=selfEmbedding]";
        cout << endl;
        cout << "  - using approximate translation.";
        if (writeProtocol) cout << " [alg=approximate]";
        cout << endl;
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
        int Nj = -1;
        if (rSym >= 0) {
            Nj = this->SymToNi[rSym];
        }
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


// temporal SCC information
int maxdfs; // counter for dfs
bool *U; // set of unvisited nodes
vector<int> *S; // stack
bool *containedS;
int *dfsI;
int *lowlink;
int numSCCs;
int *sccSize;
int numCyclicSccs;
int **sccToSym;

void CFGtoFDAtranslator::calcSCCs(int tinit) {
    cout << "- calculating SCCs..." << endl;
    maxdfs = 0;
    U = new bool[numSymbols];
    S = new vector<int>;
    containedS = new bool[numSymbols];
    dfsI = new int[numSymbols];
    lowlink = new int[numSymbols];
    numSCCs = 0;
    SymToNi = new int[numSymbols];
    for (int i = 0; i < numSymbols; i++) {
        U[i] = true;
        containedS[i] = false;
        SymToNi[i] = -1;
    }

    tarjan(tinit); // this works only if there is a single initial task and all tasks are connected to to that task

    sccSize = new int[numSCCs];
    for (int i = 0; i < numSCCs; i++)
        sccSize[i] = 0;
    numCyclicSccs = 0;
    for (int i = 0; i < numSymbols; i++) {
        int j = SymToNi[i];
        if (SymToNi[i] < 0)
            continue;
        sccSize[j]++;
        if (sccSize[j] == 2)
            numCyclicSccs++;
    }
    cout << "- number of SCCs: " << numSCCs << " [numSCCs=" << numSCCs << "]" << endl;

    // generate inverse mapping
    sccToSym = new int *[numSCCs];
    int currentI[numSCCs];
    for (int i = 0; i < numSCCs; i++)
        currentI[i] = 0;
    for (int i = 0; i < numSCCs; i++) {
        sccToSym[i] = new int[sccSize[i]];
    }
    for (int i = 0; i < numSymbols; i++) {
        int scc = SymToNi[i];
        if (scc < 0)
            continue;
        sccToSym[scc][currentI[scc]] = i;
        currentI[scc]++;
    }

    // search for sccs with size 1 that contain self-loops
    set<int> selfLoopSccs;
    for (int i = 0; i < numSCCs; i++) {
        if (sccSize[i] == 1) {
            int v = sccToSym[i][0];
            if (rFirst[v] >= 0) {
                for (int j = rFirst[v]; j <= rLast[v]; j++) {
                    grRule *r = rules[j];
                    for (int k = 0; k < r->rLength; k++) {
                        int w = r->right[k];
                        if (v == w) { // this is a self loop
                            selfLoopSccs.insert(i);
                        }
                    }
                }
            }
        }
    }
    numCyclicSccs += selfLoopSccs.size();

    int *sccsCyclic = new int[numCyclicSccs];
    int j = 0;
    int sccMaxSize = -1;
    for (int i = 0; i < numSCCs; i++) {
        if (sccSize[i] > sccMaxSize)
            sccMaxSize = sccSize[i];
        if (sccSize[i] > 1) {
            sccsCyclic[j] = i;
            j++;
        }
    }
    cout << "- number of cyclic SCCs: " << numCyclicSccs << ". [cyclicSCCs=" << numCyclicSccs << "]" << endl;
    cout << "- self-loops: " << selfLoopSccs.size() << ". [sccSelfLoops=" << selfLoopSccs.size() << "]" << endl;
    for (std::set<int>::iterator it = selfLoopSccs.begin(); it != selfLoopSccs.end(); it++) {
        sccsCyclic[j] = *it;
        j++;
    }

    NumNis = numCyclicSccs;
    NiSize = new int[numCyclicSccs];
    Ni = new int *[numCyclicSccs];
    SymToNi = new int[numSymbols];
    for (int i = 0; i < numSymbols; i++) {
        SymToNi[i] = -1; // init as non-recursive
    }

    cout << "- collecting SCC data" << endl;
    for (int k = 0; k < numCyclicSccs; k++) {
        int scc = sccsCyclic[k];
        NiSize[k] = sccSize[scc];
        Ni[k] = new int[NiSize[k]];
        for (int j = 0; j < sccSize[scc]; j++) {
            Ni[k][j] = sccToSym[scc][j];
            SymToNi[Ni[k][j]] = k;
        }
    }

    delete[] U;
    delete S;
    delete[] containedS;
    delete[] dfsI;
    delete[] lowlink;
}

void CFGtoFDAtranslator::tarjan(int v) {
    assert(v < numSymbols);
    assert(v >= 0);

    dfsI[v] = maxdfs;
    lowlink[v] = maxdfs; // v.lowlink <= v.dfs
    maxdfs++;

    S->push_back(v);
    containedS[v] = true;
    U[v] = false; // delete v from U

    if (rFirst[v] >= 0) {
        for (int i = rFirst[v]; i <= rLast[v]; i++) { // iterate rules
            assert(i >= 0);
            assert(i < numRules);
            grRule *r = rules[i];
            for (int j = 0; j < r->rLength; j++) {
                int w = r->right[j];
                if (w < 0)
                    continue; // epsilon transition
                if (U[w]) {
                    tarjan(w);
                    lowlink[v] = min(lowlink[v], lowlink[w]);
                } else if (containedS[w]) {
                    lowlink[v] = min(lowlink[v], dfsI[w]);
                }
            }
        }
    }

    if (lowlink[v] == dfsI[v]) { // root of an SCC
        int v2;
        do {
            v2 = S->back();
            S->pop_back();
            containedS[v2] = false;
            SymToNi[v2] = numSCCs;
        } while (v2 != v);
        numSCCs++;
    }
}

CFGtoFDAtranslator::~CFGtoFDAtranslator() {
    delete[] NiSize;
    for (int i = 0; i < numCyclicSccs; i++) {
        delete[] Ni[i];
    }
    delete[] Ni;
    delete[] SymToNi;
    delete dfa;
    delete[] NiRec;
}

void CFGtoFDAtranslator::printRules() {
    for (int i = 0; i < numRules; i++) {
        grRule *r = rules[i];
        printRule(r);
    }
}


// *************************************************
// * New Stuff
// *************************************************

FA * CFGtoFDAtranslator::makeFABU(Model *htn, int tinit) {
    // construct SCCs and topsort of SCCs
    /*calcSCCs(tinit);
    sccs = new vector<int>[numSCCs];
    for (int i = numTerminals; i < numSymbols; i++) {
        sccs[SymToNi[i]].push_back(i);
    }*/
    //recursion = getRecInfo(sccs);

/*
    cout << "SCC info" << endl;
    cout <<  NumNis << endl;
    for (int i = 0; i < NumNis; i++) {
        cout << "N" << i << " " << NiRec[i] << endl;
    }

    for(int i = 0; i < numSymbols; i++) {
        cout << "sym " << i << " " << SymToNi[i] << endl;
    }
*/

//    exit(1);

    FA* fa = getFA(tinit);
    //cout << fa->sGoal.size() << " " << fa->sInit.size() << endl;
    cout << "toDFA " << fa->delta->numTransitions << " -> ";
    fa->compileToDFA();
    cout << fa->delta->numTransitions << endl;
//    fa->showDOT(htn->taskNames);
    //fa->showDOT();
    cout << "State-pruning " << fa->delta->numTransitions << " -> ";
    statePruning(htn, fa);
    cout << fa->delta->numTransitions << endl;
    //fa->compileToDFA();
    //cout << fa->delta->numTransitions << " -> ";
    //fa->showDOT();
    cout << "Minimization " << fa->delta->numTransitions << " -> ";
    fa->minimize();
    cout << fa->delta->numTransitions << endl;
    //fa->showDOT();
    //fa->showDOT(htn->taskNames);
    assert(fa->numStatesCorrect());
    return fa;
}

FA *CFGtoFDAtranslator::getFA(tLabelID alpha) {
    int Nj = SymToNi[alpha];
    FA *fa;
    if (SymToNi[alpha] == -1) {
        fa = getFaNonRec(alpha);
    } else if (NiRec[Nj] == recLeft) {
        fa = getFaLeftRec(alpha);
    } else {
        fa = getFaRightRec(alpha);
    }
//    fa->compileToDFA();
    //fa->showDOT();

    // replace non-terminal symbols
//    fa->delta->ensureFW(); // from -> (label -> to)
//    for (auto from : *fa->delta->forward) {
//        for (auto label = from.second->begin(); label != from.second->end();) {
//            if (label->first >= this->numTerminals) {
//                assert(label->second->size() == 1);
//
//                tStateID f = from.first;
//                tLabelID l = label->first;
//                tStateID t = *label->second->begin();
//
//                delete label->second;
//                label = from.second->erase(label);
//
//                FA *fa2 = getFA(l);
//                assert(fa2->isPrimitive(numTerminals));
//                fa->addSubFA(f, fa2, t);
//            } else {
//                label++;
//            }
//        }
//    }
   // assert(fa->isPrimitive(numTerminals));
    return fa;
}

FA *CFGtoFDAtranslator::getFaNonRec(tLabelID A) {
    FA* fa = new FA();
    int s0 = 0;
    int sF = 1;
    fa->sInit.insert(s0);
    fa->sGoal.insert(sF);
    fa->numSymbols = numSymbols;

    fa->numStates = 2;
    for (int j = rFirst[A]; j <= rLast[A]; j++) {
        assert(rules[j]->left == A);
        //printRule(rules[j]);
        int lastState = s0;
        for (int k = 0; k < rules[j]->rLength; k++) {
            int label = rules[j]->right[k];
            if (k < rules[j]->rLength - 1) {
                int newState = fa->numStates;
                fa->numStates++;
                if (this->isTerminalSym(label)) {
                    fa->addRule(lastState, label, newState);
                } else {
                    FA* subFA = getFA(label);
                    fa->addSubFA(lastState, subFA, newState);
                }
                lastState = newState;
            } else {
                if (this->isTerminalSym(label)) {
                    fa->addRule(lastState, label, sF);
                } else {
                    FA* subFA = getFA(label);
                    fa->addSubFA(lastState, subFA, sF);
                }
            }
        }
    }
    if (!fa->numStatesCorrect()) {
        fa->showDOT();
        fa->numStatesCorrect();
    }
    //fa->showDOT();
    assert(fa->numStatesCorrect());
    return fa;
}

FA *CFGtoFDAtranslator::getFaLeftRec(tLabelID A) {
    int Nj = SymToNi[A];
    FA *fa = new FA();

    int s0 = 0;
    fa->sInit.insert(s0);
    fa->numSymbols = numSymbols;
    map<int, int> symToState;
    symToState.insert({A, 1});
    fa->numStates = 2;

    for (int i = 0; i < NiSize[Nj]; i++) { // create new state features
        int C = Ni[Nj][i];
        if (C != A) {
            symToState.insert({C, fa->numStates++});
        }
    }
    fa->sGoal.insert(symToState[A]);

    for (int i = 0; i < NiSize[Nj]; i++) {
        int C = Ni[Nj][i];
        for (int l = rFirst[C]; l <= rLast[C]; l++) {
            // need to detect whether this is a recursive method or not
            grRule *rule = rules[l];
            //printRule(rule);
            assert(rule->left == C);
            assert(SymToNi[C] == Nj);
            int D = rule->right[0]; // first right-hand side
            int Nk = -1;
            if (D >= 0) { // might be epsilon
                Nk = SymToNi[D];
            }
            int k;
            int lastState;
            if (Nj != Nk) { // non-recursive
                k = 0;
                lastState = s0;
            } else { // recursive
                k = 1; // first one is in same Nj
                lastState = symToState[D];
            }
            for (; k < rule->rLength; k++) {
                int label = rule->right[k];
                if (k < rule->rLength - 1) {
                    int newState = fa->numStates;
                    fa->numStates++;
                    if (this->isTerminalSym(label)) {
                        fa->addRule(lastState, label, newState);
                    } else {
                        FA* subFA = getFA(label);
                        fa->addSubFA(lastState, subFA, newState);
                    }
                    lastState = newState;
                } else {
                    if (this->isTerminalSym(label)) {
                        fa->addRule(lastState, label, symToState[C]);
                    } else {
                        FA* subFA = getFA(label);
                        fa->addSubFA(lastState, subFA, symToState[C]);
                    }
                }
            }
        }
    }
    if (!fa->numStatesCorrect()) {
        fa->showDOT();
        fa->numStatesCorrect();
    }
    //fa->showDOT();
    assert(fa->numStatesCorrect());
    return fa;
}

FA * CFGtoFDAtranslator::getFaRightRec(tLabelID A) {
    int Nj =  SymToNi[A];
    FA *fa = new FA();
    fa->numSymbols = numSymbols;

    map<int, int> symToState;
    int sF = 1;
    fa->sGoal.insert(sF);
    symToState.insert({A, 0});
    fa->numStates = 2;

    for (int i = 0; i < NiSize[Nj]; i++) { // create new state features
        int C = Ni[Nj][i];
        if (C != A) {
            symToState.insert({C, fa->numStates++});
        }
    }

    int s0 = symToState[A];
    fa->sInit.insert(s0);

    for (int i = 0; i < NiSize[Nj]; i++) {
        int C = Ni[Nj][i];
        for (int l = rFirst[C]; l <= rLast[C]; l++) {
            // need to detect whether this is a recursive method or not
            grRule *rule = rules[l];
            //printRule(rule);
            assert(rule->left == C);
            assert(SymToNi[C] == Nj);
            int D = rule->right[rule->rLength - 1]; // last right-hand side
            int Nk = -1;
            if (D >= 0) { // might be epsilon
                Nk = SymToNi[D];
            }
            bool isRecursive = (Nj == Nk);
            int proceedTo = rule->rLength;
            if (isRecursive) { // last symbol not processed
                proceedTo--;
            }
            int lastState = symToState[C];
            for (int k = 0; k < proceedTo; k++) {
                int label = rule->right[k];
                if (k < proceedTo - 1) {
                    int newState = fa->numStates;
                    fa->numStates++;
                    if (this->isTerminalSym(label)) {
                        fa->addRule(lastState, label, newState);
                    } else {
                        FA* subFA = getFA(label);
                        fa->addSubFA(lastState, subFA, newState);
                    }
                    lastState = newState;
                } else {
                    if (!isRecursive) { // non-recursive
                        if (this->isTerminalSym(label)) {
                            fa->addRule(lastState, label, sF);
                        } else {
                            FA* subFA = getFA(label);
                            fa->addSubFA(lastState, subFA, sF);
                        }
                    } else { // recursive
                        if (this->isTerminalSym(label)) {
                            fa->addRule(lastState, label, symToState[D]);
                        } else {
                            FA* subFA = getFA(label);
                            fa->addSubFA(lastState, subFA, symToState[D]);
                        }
                    }
                }
            }
        }
    }
    if (!fa->numStatesCorrect()) {
        fa->showDOT();
        fa->numStatesCorrect();
    }
    //fa->showDOT();
    assert(fa->numStatesCorrect());

    return fa;
}

void CFGtoFDAtranslator::statePruning(Model *htn, FA *pFa) {
    pFa->delta->ensureBW();
    unordered_map<tStateID, set<int>*> pStates; // partial state that holds with certainty
    for (auto l : *pFa->delta->backward) {
        for (auto to : *l.second) {
            if (pStates.find(to.first) == pStates.end()) {
                set<int>* pState = new set<int>;
                pStates.insert({to.first, pState});
                int action = l.first;

                // preconditions that are not changed
                for (int i = 0; i < htn->numPrecs[action]; i++) {
                    int prec = htn->precLists[action][i];
                    pState->insert(prec);
                }
                // effects
                for (int i = 0; i < htn->numDels[action]; i++) {
                    int del = htn->delLists[action][i];
                    pState->erase(del);
                }
                for (int i = 0; i < htn->numAdds[action]; i++) {
                    int add = htn->addLists[action][i];
                    pState->insert(add);
                }
            } else {
                set<int>* pState = pStates.find(to.first)->second;
                if (pState == nullptr) {
                    continue;
                }
                // intersect old set with (unchanged precs \cup add effects)
                set<int>* tempPState = new set<int>;
                int action = l.first;

                // preconditions that are not changed
                for (int i = 0; i < htn->numPrecs[action]; i++) {
                    int prec = htn->precLists[action][i];
                    tempPState->insert(prec);
                }
                // effects
                for (int i = 0; i < htn->numDels[action]; i++) {
                    int del = htn->delLists[action][i];
                    tempPState->erase(del);
                }
                for (int i = 0; i < htn->numAdds[action]; i++) {
                    int add = htn->addLists[action][i];
                    tempPState->insert(add);
                }
                for(auto iter = pState->begin(); iter != pState->end(); ) {
                    int p = *iter;
                    if (tempPState->find(p) == tempPState->end()) {
                        iter = pState->erase(iter);
                    } else {
                        ++iter;
                    }
                }
                delete tempPState;

                if (pState->empty()) { // closed
                    delete pState;
                    pStates.at(to.first) = nullptr;
                }
            }
        }
    }
//    cout << endl;
//    for (auto pState : pStates) {
//        if (pState.second == nullptr) {
//            continue;
//        }
//        cout << pState.first <<  ":";
//        for(int p : *pState.second) {
//            cout << " " << htn->factStrs[p];
//        }
//         cout << endl;
//    }
//    cout << endl;

    pFa->delta->ensureFW();
    for (auto from : *pFa->delta->forward) {
        if ((pStates.find(from.first) == pStates.end()) || (pStates.find(from.first)->second == nullptr)) {
            continue;
        }

        set<int> pState = *pStates.find(from.first)->second;

        //for (auto actions : *from.second) {
        for (auto iter = from.second->begin(); iter != from.second->end(); ) {
            auto actions = *iter;
            bool applicable = true;
            int action = actions.first;
            for (int i = 0; i < htn->numPrecs[action]; i++) {
                int p = htn->precLists[action][i];
                int var =  htn->bitToVar[p];
                for (int j = htn->firstIndex[var]; j <= htn->lastIndex[var]; j++) {
                    if ((j != p) && (pState.find(j) != pState.end())) {
//                        cout << "delete n" << from.first << " action " << htn->taskNames[action] << endl;
                        applicable = false;
                        break;
                    }
                }
                if (!applicable) {
                    break;
                }
            }
            if (!applicable) {
                delete actions.second;
                iter = from.second->erase(iter);
                pFa->delta->numTransitions--;
            } else {
                iter++;
            }
        }
    }
    for (auto pState : pStates) {
        delete pState.second;
    }
}

//
//eRecursion * CFGtoFDAtranslator::getRecInfo(const vector<int> *sccs) {
//    eRecursion* recursion = new eRecursion[numSCCs];
//    for(int i = 0; i < numSCCs; i++) {
//        bool leftGen = false;
//        bool rightGen = false;
//        for (int C : sccs[i]) {
//            for (int l = rFirst[C]; l <= rLast[C]; l++) {
//                printRule(rules[l]);
//                leftGen = leftGen || rules[l]->isLeftGenerating;
//                rightGen = rightGen || rules[l]->isRightGenerating;
//                if (leftGen && rightGen) {
//                    break;
//                }
//            }
//            if (leftGen && rightGen) {
//                break;
//            }
//        }
//        if (leftGen && rightGen) {
//            recursion[i] = rSELF;
//            cout << "scc" << i << " " << "self" << endl;
//        } else if(leftGen) {
//            recursion[i] = rRIGHT;
//            cout << "scc" << i << " " << "right" << endl;
//        } else if(rightGen) {
//            recursion[i] = rLEFT;
//            cout << "scc" << i << " " << "left" << endl;
//        } else {
//            recursion[i] = rNONE;
//            //cout << "scc" << i << " " << "none" << endl;
//        }
//    }
//    return recursion;
//}


/*
// want to delete FA as early as possible, calc how long they are needed
int* keepUntil = new int[numSymbols];
for(int i = 0; i < numSymbols; i++) {
    keepUntil[i] = -1;
}
for(int i = 0; i < numSCCs; i++) {
    // go through rules, when a rule for a task in scc i uses task x, x needs to be kept until after scc i
    for (int C : sccs[i]) {
        for (int l = rFirst[C]; l <= rLast[C]; l++) {
            for (int j = 0; j < rules[l]->rLength; j++) {
                int sym = rules[l]->right[j];
                keepUntil[sym] = i;
            }
        }
    }
}
for(int i = 0; i < numSymbols; i++) {
    cout << "keep " << i << " until after scc " << keepUntil[i] << endl;
}
*/
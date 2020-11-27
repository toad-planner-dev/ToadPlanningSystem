//
// Created by dh on 28.09.20.
//

#include "CFGtoFDAtranslator.h"
#include "../ModelWriter.h"
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <map>

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
    assert(alpha->size() > 0);
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
                            makeFA(qD, copySubSeq(rule, 1, rule->rLength), qC);
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
                        if(D >= 0) {
                            Nk = SymToNi[D];
                        }
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
    /*
    cout << "- starting grammar simplification" << endl;

    // symbol to rules that decompose it
    map<int, vector<grRule *> *> *rulesDecomposing = new map<int, vector<grRule *> *>;

    // symbol to rules it is contained in the right side
    map<int, vector<grRule *> *> *containedInRight = new map<int, vector<grRule *> *>;

    // symbols that can be decomposed into an epsilon
    set<int> symBoldWithEpsilonRules;

    // collect data
    set<int> onlyOnce;
    for (int i = 0; i < tempRules.size(); i++) {
        grRule *r = tempRules[i];
        r->rSymbolsSet = 0;
        if (r->rLength == 0) {
            symBoldWithEpsilonRules.insert(r->left);
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

    if (symBoldWithEpsilonRules.size() > 0) {
        cout << "- deleting epsilon rules" << endl;
        for (int s : symBoldWithEpsilonRules) {
            if (containedInRight->find(s) == containedInRight->end()) {
                continue;
            }
            vector<grRule *> *R = rulesDecomposing->at(s);
            bool onlyDeleted = true;
            for (int i = 0; i < R->size(); i++) {
                if (R->at(i)->rLength > 0) {
                    onlyDeleted = false;
                    break;
                }
            }
            if (onlyDeleted) { // just

            }

            vector<grRule *> *rules = containedInRight->at(s);
            for (int i = 0; i < rules->size(); i++) {
                grRule *r = rules->at(i);
                vector<grRule *> multiply;
                multiply.push_back(r);
                for (int j = 0; j < r->rLength; j++) {
                    //if()
                }
            }
        }
    } else {
        cout << "- no epsilon rules in grammar" << endl;
    }
*/
    // copy rules
    this->numRules = tempRules.size();
    rules = new grRule *[numRules];
    for (int i = 0; i < tempRules.size(); i++) {
        grRule* r =tempRules.at(i);
        if(r->rLength == 0) {
            r->rLength = 1;
            r->right = new int[1];
            r->right[0] = -1;
        }
        rules[i] = r;
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
        /*
        for (int j = 0; j < NiSize[i]; j++) {
            cout << " " << Ni[i][j];
        }
        cout << endl;
         */
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
                if(w < 0)
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

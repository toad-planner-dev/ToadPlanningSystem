//
// Created by dh on 28.09.20.
//

#include "CFGtoFDAtranslator.h"
#include <cassert>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sys/time.h>

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


int CFGtoFDAtranslator::isTerminalSym(int a) {
    return (a != Epsilon) && (a < this->numTerminals);
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

void CFGtoFDAtranslator::initDataStructures(int startingSymbol) {
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
            if (onlyOnce.find(sym) == onlyOnce.end()) {
                r->rSymbolsSet++;
            }
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
    cout << "- " << deleteRules.size() << " rules are not bottom-up reachable." << endl;

    // top-down reachability
//    todo.clear();
//    todo.push_back(startingSymbol);
//
//    set<int> tdReached;
//    tdReached.insert(startingSymbol);
//    while (!todo.empty()) {
//        int s = todo.back();
//        todo.pop_back();
//        auto iter = rulesDecomposing->find(s);
//        if (iter == rulesDecomposing->end()) {
//            continue;
//        }
//        for (auto rule : *iter->second) {
//            for (int j = 0; j < rule->rLength; j++) {
//                int u = rule->right[j];
//                if (tdReached.find(u) == tdReached.end()) {
//                    tdReached.insert(u);
//                    if(!isTerminalSym(u)) {
//                        todo.push_back(u);
//                    }
//                }
//            }
//        }
//    }
//
//    cout << "- " << (this->numSymbols - tdReached.size()) << " of " << tdReached.size() << " symbols are not top down reachable." << endl;


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
            r->right[0] = Epsilon;
        }
        rules[newI++] = r;
    }

    cout << "- sorting rules...";
    sortRules();
    cout << "(done)" << endl;

    // store rules for each symbol
    rFirst = new int[numSymbols];// todo: free old structures when recomputing
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
        this->isRegular = true;
    } else if (isRegular) {
        cout << "  - the instance is recursive, but not self-embedding, i.e. it is regular.";
        if (writeProtocol) cout << " [rec=nonSelfEmbedding]";
        cout << endl;
        cout << "  - using exact translation.";
        if (writeProtocol) cout << " [alg=exact]";
        cout << endl;
        this->isRegular = true;
    } else {
        cout << "  - the instance is recursive and self-embedding, i.e. it could not be shown that it is regular.";
        if (writeProtocol) cout << " [rec=selfEmbedding]";
        cout << endl;
        cout << "  - using approximate translation.";
        if (writeProtocol) cout << " [alg=approximate]";
        cout << endl;
        this->isRegular = false;
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
        if (rSym != Epsilon) {
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
    symPreorder.clear();

    tarjan(tinit); // this works only if there is a single initial task and all tasks are connected to that task

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
                if (w == Epsilon)
                    continue;
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
            if (!isTerminalSym(v2)) {
                symPreorder.push_back(v2);
            }
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
    delete[] NiRec;
}

void CFGtoFDAtranslator::printRules() {
    for (int i = 0; i < numRules; i++) {
        grRule *r = rules[i];
        printRule(r);
    }
}


// *************************************************
// * Bottom up methods
// *************************************************

StdVectorFst *CFGtoFDAtranslator::makeFABU(Model *htn, int tinit, int inplaceThreshold) {
    detSymHandeledInplace(htn, inplaceThreshold);

    cout << "- building sub-automata..." << endl;
    unordered_map<int, const Fst<StdArc>*> subautomata;

    timeval tp;
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    long endT;

    StdVectorFst* fstInit;
    double output = 0;
    cout << "  0%" << endl;
    vector<pair<int, const Fst<StdArc>*>> label_fst_pairs;

    for (int i = 0; i < symPreorder.size(); i++) {
        double current = 100.0 / symPreorder.size() * i;
        if (current > (output + 10)) {
            gettimeofday(&tp, NULL);
            endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
            cout << " " << fixed << setprecision(0) << current << "% [" << fixed << setprecision(2) << (endT - startT) << "]" << endl;
            output += 10;
        }

        const int sym = symPreorder[i];
        if (inplace[sym]) continue;

        tstack.clear();
        StdVectorFst *fst = tdMakeFA(sym);

        RmEpsilon(fst);
        StdVectorFst *fst2 = getNewFA();
        Determinize(*fst, fst2);
        delete fst;
        fst = fst2;
        Minimize(fst);

        label_fst_pairs.emplace_back(sym + 1, fst);

        if (!tstack.empty())  { // combine
            StdVectorFst *fstFull = getNewFA();
            Replace(label_fst_pairs, fstFull, sym + 1, true);
            label_fst_pairs.erase(label_fst_pairs.end());
            delete fst;
            fst = fstFull;

            RmEpsilon(fst);
            fst2 = getNewFA();
            Determinize(*fst, fst2);
            delete fst;
            fst = fst2;
            Minimize(fst);
            label_fst_pairs.emplace_back(sym + 1, fst);
        }
        if (sym == tinit) {
            fstInit = fst;
            assert(i == (symPreorder.size() - 1));
        }
    }

    for (auto subFAs : label_fst_pairs) {
        if (subFAs.first != (tinit + 1)) {
            delete subFAs.second;
        }
    }

    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "100%" << " [timeBuildFA=" << (endT - startT) << "]" << endl;

    cout << "final automaton has " << (int) fstInit->NumStates() << " states [faFinalStates=" << (int) fstInit->NumStates() << "]." << endl;
    return fstInit;
}

void CFGtoFDAtranslator::addRule(StdVectorFst *fst, int from, int label, int to, int costs) {
    fst->AddArc(from, StdArc(label + 1, label + 1, 0, to));
}

int CFGtoFDAtranslator::nextState(StdVectorFst *fst) {
    fst->AddState();
    return fst->NumStates() - 1;
}

StdVectorFst *CFGtoFDAtranslator::getNewFA() {
    StdVectorFst *fst = new StdVectorFst();
    fst->SetProperties(kAcceptor, true);
    return fst;
}


StdVectorFst *CFGtoFDAtranslator::makeFATD(Model *htn, int init, int inplaceThreshold, bool interOpt) {
    detSymHandeledInplace(htn, inplaceThreshold);
    if (interOpt) {
        cout << "- using intermediate optimization" << endl;
    } else {
        cout << "- not using intermediate optimization" << endl;
    }

    vector<pair<int, const Fst<StdArc>*>> label_fst_pairs;
    tstack.clear();
    done.clear();
    tstack.push_back(init);
    done.insert(init);

    cout << "- building sub-FAs" << endl;
    timeval tp;
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    while (!tstack.empty()) {
        int task = tstack[tstack.size() - 1];
        tstack.pop_back();

        StdVectorFst *fst = getNewFA();
        int sInit = nextState(fst);
        fst->SetStart(sInit);
        int sFinal = nextState(fst);
        fst->SetFinal(sFinal, 0);
        tdMakeFA(fst, sInit, task, sFinal, true);

        if (interOpt) {
            RmEpsilon(fst);
            StdVectorFst *fst2 = getNewFA();
            Determinize(*fst, fst2);
            delete fst;
            fst = fst2;
            Minimize(fst);
        }
        label_fst_pairs.emplace_back(task + 1, fst);
    }
    cout << "  - [numSubFAs=" << label_fst_pairs.size() << "]" << endl;
    gettimeofday(&tp, NULL);
    long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "  - [timeBuildingSubFAs=" << (endT - startT) << "]" << endl;
    startT = endT;

    cout << "- combining sub-FAs.";
    StdVectorFst* fst = getNewFA();
    Replace(label_fst_pairs, fst, htn->initialTask + 1, true);
    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << " [timeCombiningSubFAs=" << (endT - startT) << "]" << endl;
    cout << "- [numStatesRAW=" << fst->NumStates() << "]" << endl;
    return fst;
}

void CFGtoFDAtranslator::detSymHandeledInplace(const Model *htn, int inplaceThreshold) {
    cout << "- inplace threshold [ipt=" << inplaceThreshold  << "]" << endl;
    if (inplaceThreshold == -1) {
        inplace.resize(numSymbols, true);
        cout << "- ALL tasks handled inplace" << endl;
        return;
    } else if  (inplaceThreshold == -2) {
        inplace.resize(numSymbols, false);
        cout << "- NO tasks handled inplace" << endl;
        return;
    }
    unordered_map<int, int>* occurances = new unordered_map<int, int>();
    for (int i = numTerminals; i < numSymbols; i++) {
        occurances->insert(make_pair(i, 0));
    }
    for (int i = 0; i < this->numRules; i++) {
        for (int j = 0; j < rules[i]->rLength; j++) {
            const int t = rules[i]->right[j];
            if ((t != Epsilon) && (!isTerminalSym(t))) {
                occurances->at(t)++;
            }
        }
    }
//    vector<int> hist;
//    for (int i = numTerminals; i < numSymbols; i++) {
//        int occ = occurances->at(i);
//        while (occ >= hist.size()) {
//            hist.push_back(0);
//        }
//        hist[occ]++;
//    }
//    for (int i = 0; i < hist.size(); i++) {
//        cout << i << ": " << hist[i] << " times" << endl;
//    }
    inplace.resize(numSymbols, false);
    for (int i = numTerminals; i <numSymbols; i++) {
        inplace[i] = (occurances->at(i) < inplaceThreshold);
    }
    inplace[htn->initialTask] = false; // this does not make sense
}

StdVectorFst * CFGtoFDAtranslator::tdMakeFA(int task) {
    StdVectorFst *fst = getNewFA();
    int sInit = nextState(fst);
    fst->SetStart(sInit);
    int sFinal = nextState(fst);
    fst->SetFinal(sFinal, 0);
    tdMakeFA(fst, sInit, task, sFinal, true);
    return fst;
}

void CFGtoFDAtranslator::tdMakeFA(StdVectorFst *fst, int q0, vector<int> *alpha, int q1) {
    assert(alpha->size() > 0);
    if (alpha->size() == 1) {
        tdMakeFA(fst, q0, alpha->at(0), q1);
    } else if (alpha->size() > 1) {
        int q = nextState(fst);
        int X = alpha->at(0);
        vector<int> *beta = new vector<int>;
        for (int i = 1; i < alpha->size(); i++)
            beta->push_back(alpha->at(i));
        tdMakeFA(fst, q0, X, q);
        tdMakeFA(fst, q, beta, q1);
    }
    delete alpha;
}

void CFGtoFDAtranslator::tdMakeFA(StdVectorFst *fst, int q0, int A, int q1) {
    tdMakeFA(fst, q0, A, q1, false);
}

void CFGtoFDAtranslator::tdMakeFA(StdVectorFst *fst, int q0, int A, int q1, bool top) {
    if (A == Epsilon) {
        addRule(fst, q0, A, q1, 0);
    } else if (isTerminalSym(A)) {
        addRule(fst, q0, A, q1, 1);
    } else if ((!top) &&(!inplace[A])) {
        addRule(fst, q0, A, q1, 1);
        if (done.find(A) == done.end()) {
            done.insert(A);
            tstack.push_back(A);
        }
    } else { // inplace
        if (SymToNi[A] >= 0) {
            // get partition containing A
            int Nl = SymToNi[A];

            // create new states
            unordered_map<int, int> qB;
            for (int j = 0; j < NiSize[Nl]; j++) {
                int task = Ni[Nl][j];
                int id = fst->AddState();
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
                            tdMakeFA(fst, q0, copySubSeq(rule, 0, rule->rLength), qC);
                        } else {
                            int qD = qB[D];
                            if(rule->rLength > 1) {
                                tdMakeFA(fst, qD, copySubSeq(rule, 1, rule->rLength), qC);
                            } else {
                                tdMakeFA(fst, qD, Epsilon, qC);
                            }
                        }
                    }
                }
                int qA = qB[A];
                addRule(fst,qA, Epsilon, q1, 0);
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
                            tdMakeFA(fst, qC, copySubSeq(rule, 0, rule->rLength), q1);
                        } else {
                            int qD = qB[D];
                            if(rule->rLength > 1) {
                                tdMakeFA(fst, qC, copySubSeq(rule, 0, rule->rLength - 1), qD);
                            } else {
                                tdMakeFA(fst, qC, Epsilon, qD);
                            }
                        }
                    }
                }
                int qA = qB[A];
                addRule(fst, q0, Epsilon, qA, 1);
            }
        } else { // a non-recursive non-terminal
            for (int l = rFirst[A]; l <= rLast[A]; l++) {
                grRule *rule = rules[l];
                tdMakeFA(fst, q0, copySubSeq(rule, 0, rule->rLength), q1);
            }
        }
    }
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

void CFGtoFDAtranslator::showDOT(StdVectorFst *fst) {
    system("rm binary.fst");
    fst->Write("binary.fst");
    system("fstdraw binary.fst binary.dot"); //  --acceptor=true
    system("xdot binary.dot &");
}

void CFGtoFDAtranslator::showDOT(StdVectorFst *fst, string *taskNames) {
    ofstream myfile;
    myfile.open("syms.txt");
    myfile << "<eps> 0\n";
    for(int i = 0; i < this->numSymbols; i++) {
        myfile << taskNames[i] << " " << (i + 1) << "\n";
    }
    myfile.close();

    system("rm binary.fst");
    fst->Write("binary.fst");
    system("fstdraw --acceptor=true --isymbols=syms.txt --osymbols=syms.txt binary.fst binary.dot");
    system("xdot binary.dot &");
}

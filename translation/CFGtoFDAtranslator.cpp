//
// Created by dh on 28.09.20.
//

#include "CFGtoFDAtranslator.h"
#include <cassert>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include "../utils/IntPairHeap.h"
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
            r->right[0] = epsilon;
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
        if (rSym != epsilon) {
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

    for (int i = 0; i < numSymbols; i++) {
        taskToScc.push_back(-1);
    }

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
                if (w == epsilon)
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
            taskToScc[v2] = numSCCs;
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

bool reduceSubFAs = true;
bool reduceFinalFA = false;
bool statebasedPruning = false;
bool removeEpsilon = false;

StdVectorFst *CFGtoFDAtranslator::makeFABU(Model *htn, int tinit) {
    timeval tp;
    cout << "- building sub-automata..." << endl;
    vector<pair<int, const Fst<StdArc>*>> label_fst_pairs;

    //cout << "init " << htn->initialTask << endl;
    int maxScc = -1;
    unordered_map<int, vector<int>> sccToTask;
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    long endT;
    for (int i = 0; i < numSymbols; i++) {
//        cout << i <<" -> " << taskToScc[i] << endl;
        int scc = taskToScc[i];
        if (scc > maxScc) {
            maxScc = scc;
        }
        sccToTask[scc].push_back(i);
    }
//    cout << "scc "  << numSCCs << endl;

//    int initalTask = sccToTask[maxScc][0];
//    assert(initalTask == htn->initialTask);
//    StdVectorFst* fst = getFA(initalTask);

    StdVectorFst* fstInit;
    double output = 0;
    cout << "  0%" << endl;
//    cout << "SCC:";
    for (int i = 0; i <= maxScc; i++) {
//        cout << " " << i;
        for (int task : sccToTask[i]) {
//            cout << task << endl;
            if (task < htn->numActions)
                continue;
            StdVectorFst* fst = getFA(task);
            StdVectorFst *fst2 = getNewFA();
            Determinize(*fst, fst2);
            delete fst;
            fst = fst2;
            Minimize(fst);
//            showDOT(fst);
            label_fst_pairs.emplace_back(task + 1, fst);

            StdVectorFst* fstFull = getNewFA();
            Replace(label_fst_pairs, fstFull, task + 1, true);
            fst2 = getNewFA();
            Determinize(*fstFull, fst2);
            delete fstFull;
            fstFull = fst2;
            Minimize(fstFull);
            RmEpsilon(fstFull);

            label_fst_pairs.erase(label_fst_pairs.end());
            label_fst_pairs.emplace_back(task + 1, fstFull);
            if (task == htn->initialTask) {
                fstInit = fstFull;
            }
            //showDOT(fstFull);
        }
        double current = 100.0/maxScc * i;
        if (current > (output + 10)) {
            gettimeofday(&tp, NULL);
            endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
            cout << " " << fixed << setprecision(0) << current << "% ["<< fixed << setprecision(2) << (endT - startT) << "]" << endl;
            output += 10;
        }
    }
    for (auto p : label_fst_pairs) {
        if (p.first != (htn->initialTask + 1)) {
            delete p.second;
        }
    }
    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "100%" << " [timeBuildFA=" << (endT - startT) << "]" << endl;
    startT = endT;

//    showDOT(fstInit);
//    int start = (int)fstInit->NumStates();
//    cout << "  - automaton has " << (int) fstInit->NumStates() << " states [faFullStates=" << (int) fstInit->NumStates() << "]." << endl;
//
//    //ArcMap(fstInit, RmWeightMapper<StdArc>());
//    showDOT(fstInit);
//    vector<TropicalWeightTpl<float>>* dist = new vector<TropicalWeightTpl<float>>();
//    ShortestDistance(*fstInit, dist, true);
//    for (int i = 0; i < dist->size(); i++) {
//        cout << i << " " <<fixed << setprecision(2) <<  dist->at(i).Value() << endl;
//    }


    if (statebasedPruning) {
        int startState = (int) fstInit->NumStates();
        cout << "State-based Pruning" << endl;
//        statePruning(htn, fstInit);
        statePruning2(htn, fstInit);
//        Connect(fstInit);
        Minimize(fstInit);
        int endState = (int) fstInit->NumStates();
        double reduction = (100.0 / (double) startState * (double) endState);
        cout << "- reduces states from " << startState << " to " << endState << " states [faStatebasedPruning=";
        cout << fixed << setprecision(2)  << reduction << "]." << endl;

        gettimeofday(&tp, NULL);
        endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "- [timeStateBasedPruning=" << (endT - startT) << "]" << endl;
        startT = endT;
    }

    cout << "Creating heuristic lookup table" << endl;
    int numStates = fstInit->NumStates();
    set<int> final;
    StdVectorFst* fstRev = getNewFA();
    for (int i = 0; i < numStates; i++) {
        fstRev->AddState();
    }
    for (StateIterator<StdVectorFst> siter(*fstInit); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        if(fstInit->Final(state_id) == 0) {
            final.insert(state_id);
        }
        for (ArcIterator<StdFst> aiter(*fstInit, state_id); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            addRule(fstRev,arc.nextstate, arc.ilabel, state_id, 0);
        }
    }

//    showDOT(fstRev);
    int* hVals = new int[numStates];
    for (int i = 0; i < numStates; i++) {
        hVals[i] = INT_MAX;
    }
    IntPairHeap heap(1000);
    for(int sF : final) {
        heap.add(0, sF);
    }
    while (!heap.isEmpty()) {
        int costs = heap.topKey();
        int state = heap.topVal();
        heap.pop();
        if (hVals[state] <= costs) {
            continue;
        }
        hVals[state] = costs;
        for (ArcIterator<StdFst> aiter(*fstRev, state); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int newState = arc.nextstate;
            int arcCosts = 1;
            if (arc.ilabel == 0) {
                arcCosts = 0;
            }
            int newCosts = costs + arcCosts;
            if (newCosts < hVals[newState]) {
                heap.add(newCosts, newState);
            }
        }
    }
//    for (int i = 0; i < numStates; i++) {
//        cout << i << " " << hVals[i] << endl;
//    }

    ofstream hfile;
    hfile.open("dfad.heuristic");
    hfile << numStates << "\n";
    for (int i = 0; i < numStates; i++) {
        hfile << hVals[i] << "\n";
    }
    delete hVals;
    hfile.close();
    delete fstRev;
    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [hTable=" << (endT - startT) << "]" << endl;
    startT = endT;

//    unordered_map<int, StdVectorFst*> subFAs;
//    double reduction = 0;
//    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//    for (int i = this->numTerminals; i < this->numSymbols; i++) {
//        StdVectorFst* fst = getFA(i);
//        if (reduceSubFAs) {
//            int start = (int) fst->NumStates();
//            StdVectorFst *fst2 = getNewFA();
//            Determinize(*fst, fst2);
//            delete fst;
//            fst = fst2;
//            Minimize(fst);
//            reduction += 100.0 / start * (int) fst->NumStates();
//            if (i == this->numSymbols - 1) {
//                if (reduceSubFAs) {
//                    reduction /= (double) (numSymbols - numTerminals);
//                    cout << "  - [avSizeReduction=" << fixed << setprecision(2) << (100.0 - reduction) << "]" << endl;
//                } else {
//                    cout << "  - optimization of sub-automata disabled." << endl;
//                }
//            }
//        }
//        //showDOT(fst, htn->taskNames);
//        label_fst_pairs.emplace_back(i + 1, fst);
//        subFAs[i + 1] = fst;
//    }
//    long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//    cout << "- [timeBuildSubFAs=" << (endT - startT) << "]" << endl;
//    startT = endT;
//
//    cout << "- combining sub-automata..." << endl;
//    set<int> include;
//    for (int i = this->numTerminals; i < this->numSymbols; i++) {
//        StdVectorFst *fst = subFAs[i];
//        bool onlyPrim = true;
//        for (StateIterator<StdVectorFst> siter(*fst); !siter.Done(); siter.Next()) {
//            int state_id = siter.Value();
//            for (ArcIterator<StdFst> aiter(*fst, state_id); !aiter.Done(); aiter.Next()) {
//                const StdArc &arc = aiter.Value();
//                const int action = arc.ilabel;
//                if (action >= htn->numActions) {
//                    onlyPrim = false;
//                    break;
//                }
//            }
//            if (!onlyPrim) {
//                break;
//            }
//        }
//        if (onlyPrim) {
//            include.insert(i);
//        }
//    }
//    // now include contains nonterminals mapped to solely primitives
////    while (true) {
////        label_fst_pairs.clear();
////
////
////    }
//    StdVectorFst* fst = getNewFA();
//    Replace(label_fst_pairs, fst, htn->initialTask + 1, true);
//    for (auto subFA : label_fst_pairs) {
//        delete subFA.second;
//    }
//    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//    cout << "- [timeBuildFA=" << (endT - startT) << "]" << endl;
//    startT = endT;
//
//    int start = (int)fst->NumStates();
//    cout << "  - automaton has " << (int) fst->NumStates() << " states [faFullStates=" << (int) fst->NumStates() << "]." << endl;
//    if (statebasedPruning || removeEpsilon) {
//        cout << "- remove epsilons" << endl;
//        RmEpsilon(fst);
//        endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//        cout << "- [timeRemoveEpsilon=" << (endT - startT) << "]" << endl;
//        startT = endT;
//    }
//    if (reduceFinalFA || statebasedPruning) {
//        cout << "- make deterministic" << endl;
//        StdVectorFst *fst2 = getNewFA();
//        Determinize(*fst, fst2);
//        delete fst;
//        fst = fst2;
//        endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//        cout << "- [timeDeterminize=" << (endT - startT) << "]" << endl;
//        startT = endT;
//
//        cout << "  - automaton has " << (int) fst->NumStates() << " states." << endl;
//        cout << "- minimize automaton" << endl;
//        Minimize(fst);
//        cout << "- automaton has " << (int) fst->NumStates() << " states [faFinalStates=" << (int) fst->NumStates() << "]." << endl;
////        reduction = 100.0 / start * (int) fst->NumStates();
//        endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//        cout << "- [timeMinimize=" << (endT - startT) << "]" << endl;
//        startT = endT;
//    }
////    StdVectorFst* fst3 = getNewFA();
////    ShortestPath(*fa->fst, fst3, 10);
////    fa->fst = fst3;
////    cout << "- automaton has " << (int)fst->NumStates() << " states." << endl;
////    showDOT(fst, htn->taskNames);
////    showDOT(fst);

//    if (reduceFinalFA || statebasedPruning) {
//        cout << "- [finalSizeReduction=" << fixed << setprecision(2) << (100.0 - reduction) << "]" << endl;
//    }
//    showDOT(fst, htn->taskNames);
    cout << "final automaton has " << (int) fstInit->NumStates() << " states [faFinalStates=" << (int) fstInit->NumStates() << "]." << endl;
    return fstInit;
}

StdVectorFst *CFGtoFDAtranslator::getFA(tLabelID alpha) {
    //cout << "processing alpha " << alpha << endl;
    totalSubFAs++;
    StdVectorFst *fa = getNewFA();
    if (alpha == Epsilon) {
        fa->AddState();
        fa->AddState();
        fa->SetStart(0);
        fa->SetFinal(1, 0);
        addRule(fa, 0, epsilon, 1, 0);
    } else {
        int Nj = SymToNi[alpha];
        if (SymToNi[alpha] == -1) {
            fa = getFaNonRec(alpha);
        } else if (NiRec[Nj] == recLeft) {
            fa = getFaLeftRec(alpha);
        } else {
            fa = getFaRightRec(alpha);
        }
    }
    return fa;
}

StdVectorFst *CFGtoFDAtranslator::getFaNonRec(tLabelID A) {
    StdVectorFst* fa = getNewFA();
    int s0 = nextState(fa);
    fa->SetStart(s0);
    int sF = nextState(fa);
    fa->SetFinal(sF, 0);

    for (int j = rFirst[A]; j <= rLast[A]; j++) {
//          printRule(rules[j]);
        assert(!isTerminalSym(A));
        assert(rules[j]->left == A);
        int lastState = s0;
        for (int k = 0; k < rules[j]->rLength; k++) {
            int label = rules[j]->right[k];
            if (k < rules[j]->rLength - 1) {
                int newState = nextState(fa);
                addRule(fa, lastState, label, newState, 1);
                lastState = newState;
            } else {
                addRule(fa, lastState, label, sF, 1);
            }
        }
    }
    return fa;
}

StdVectorFst *CFGtoFDAtranslator::getFaLeftRec(tLabelID A) {
    int Nj = SymToNi[A];
    StdVectorFst *fa = getNewFA();

    int s0 = nextState(fa);
    fa->SetStart(s0);
    int sF = nextState(fa);
    fa->SetFinal(sF, 0);

    map<int, int> symToState;
    symToState.insert({A, sF});
    for (int i = 0; i < NiSize[Nj]; i++) { // create new state features
        int C = Ni[Nj][i];
        if (C != A) {
            symToState.insert({C, nextState(fa)});
        }
    }

    for (int i = 0; i < NiSize[Nj]; i++) {
        int C = Ni[Nj][i];
        for (int l = rFirst[C]; l <= rLast[C]; l++) {
            // need to detect whether this is a recursive method or not
            grRule *rule = rules[l];
            assert(rule->left == C);
            assert(SymToNi[C] == Nj);
            int D = rule->right[0]; // first right-hand side
            int Nk = -1;
            if (D != epsilon) {
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

                // when the only sign is also in the SCC, the "rest" is empty
                if (rule->rLength == 1) {
                    addRule(fa, symToState[D], epsilon, symToState[C], 1);
                }
            }
            for (; k < rule->rLength; k++) {
                int label = rule->right[k];
                int target;
                if (k < rule->rLength - 1) {
                    target = nextState(fa);
                } else {
                    target = symToState[C];
                }
                addRule(fa, lastState, label, target, 1);
                lastState = target;
            }
        }
    }
    return fa;
}

StdVectorFst * CFGtoFDAtranslator::getFaRightRec(tLabelID A) {
    int Nj = SymToNi[A];
    StdVectorFst *fa = getNewFA();

    map<int, int> symToState;
    int s0 = nextState(fa);
    fa->SetStart(s0);
    int sF = nextState(fa);
    fa->SetFinal(sF, 0);

    symToState.insert({A, s0});

    for (int i = 0; i < NiSize[Nj]; i++) { // create new state features
        int C = Ni[Nj][i];
        if (C != A) {
            symToState.insert({C, nextState(fa)});
        }
    }

    for (int i = 0; i < NiSize[Nj]; i++) {
        int C = Ni[Nj][i];
        for (int l = rFirst[C]; l <= rLast[C]; l++) {
            // need to detect whether this is a recursive method or not
            grRule *rule = rules[l];
//            printRule(rule);
            assert(rule->left == C);
            assert(SymToNi[C] == Nj);
            int D = rule->right[rule->rLength - 1]; // last right-hand side
            int Nk = -1;
            if (D != epsilon) { // might be epsilon
                Nk = SymToNi[D];
            }
            bool isRecursive = (Nj == Nk);
            int proceedTo = rule->rLength;
            if (isRecursive) { // last symbol not processed
                proceedTo--;

                // when the only sign is also in the SCC, the "rest" is empty
                if (proceedTo == 0) {
                    addRule(fa, symToState[C], epsilon, symToState[D], 1);
                }
            }
            int lastState = symToState[C];
            for (int k = 0; k < proceedTo; k++) {
                int label = rule->right[k];
                if (k < proceedTo - 1) {
                    int newState = nextState(fa);
                    addRule(fa, lastState, label, newState, 1);
                    lastState = newState;
                } else {
                    int target;
                    if (!isRecursive) {
                        target = sF;
                    } else {
                        target = symToState[D];
                    }
                    addRule(fa, lastState, label, target, 1);
                }
            }
        }
    }
    return fa;
}

void CFGtoFDAtranslator::statePruning2(Model *htn, StdVectorFst *fst) {
    cout << "- expensive state-based pruning" << endl;
//    showDOT(fst);
    const int numStates = fst->NumStates();
    // initial one dr state per dfa state
    vector<bool> *drState = new vector<bool>[numStates];
    for (int i = 0; i < numStates; i++) {
        for (int j = 0; j < htn->numStateBits; j++) {
            drState[i].push_back(false);
        }
    }
    for (int i = 0; i < htn->s0Size; i++) {
        int f = htn->s0List[i];
        drState[0][f] = true;
    }

    vector<int> fringe;
    set<int> inFringe;
    fringe.push_back(0);
    inFringe.insert(0);
    while (!fringe.empty()) {
        int s = fringe.back();
        fringe.pop_back();
        inFringe.erase(s);
        for (ArcIterator<StdFst> aiter(*fst, s); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int a = arc.ilabel - 1;
            int s2 = arc.nextstate;
            bool applicable = true;
            for (int i = 0; i < htn->numPrecs[a]; i++) {
                int f = htn->precLists[a][i];
                if (!drState[s][f]) {
                    applicable = false;
                    break;
                }
            }
            bool changed = false;
            if (applicable) {
                for (int i = 0; i < htn->numAdds[a]; i++) {
                    int f = htn->addLists[a][i];
                    if (!drState[s2][f]) {
                        drState[s2][f] = true;
                        changed = true;
                    }
                }
                for (int f = 0; f < htn->numStateBits; f++) { // progress rest of state
                    if (drState[s][f] && !drState[s2][f]) {
                        drState[s2][f] = true;
                        changed = true;
                    }
                }
            }
            if (changed) {
                if (inFringe.find(s2) == inFringe.end()) {
                    fringe.push_back(s2);
                    inFringe.insert(s2);
                }
            }
        }
    }
    // pruning
    int prunedArcs = 0;
    int otherArcs = 0;
    vector<int> arcs;
    for (int s = 0; s < numStates; s++) {
        arcs.clear();
        for (ArcIterator<StdFst> aiter(*fst, s); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int a = arc.ilabel - 1;
            bool applicable = true;
            for (int i = 0; i < htn->numPrecs[a]; i++) {
                int f = htn->precLists[a][i];
                if (!drState[s][f]) {
                    applicable = false;
                    break;
                }
            }
            if (applicable) {
                arcs.push_back(arc.ilabel - 1);
                arcs.push_back(arc.nextstate);
                otherArcs++;
            } else {
                prunedArcs++;
            }
        }
        fst->DeleteArcs(s);
        for (int i = 0; i < arcs.size(); i+=2) {
            addRule(fst, s, arcs[i], arcs[i + 1], 0);
        }
    }
    delete[] drState;
    cout << "- pruned " << prunedArcs << " of " << (prunedArcs + otherArcs) << " arcs." << endl;
//    showDOT(fst);
}

void CFGtoFDAtranslator::statePruning(Model *htn, StdVectorFst *fst) {
    const int numStates = fst->NumStates();
    set<int>* incommingActions = new set<int>[numStates];
    for (StateIterator<StdVectorFst> siter(*fst); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        for (ArcIterator<StdFst> aiter(*fst, state_id); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            incommingActions[arc.nextstate].insert(arc.ilabel - 1);
        }
    }
    set<int>* accumState = new set<int>;
    set<int>* actionInv = new set<int>;
    vector<int> arcs;
    for (int state = 0; state < numStates; state++) {
        if (state == fst->Start()) {
            continue;
        }
        accumState->clear();
        bool first = true;
        if (!incommingActions[state].empty()) {
            for (int action : incommingActions[state]) {
                actionInv->clear();
                if (action != epsilon) {
                    // preconditions that are not changed
                    for (int i = 0; i < htn->numPrecs[action]; i++) {
                        int prec = htn->precLists[action][i];
                        actionInv->insert(prec);
                    }
                    // effects
                    for (int i = 0; i < htn->numDels[action]; i++) {
                        int del = htn->delLists[action][i];
                        actionInv->erase(del);
                    }
                    for (int i = 0; i < htn->numAdds[action]; i++) {
                        int add = htn->addLists[action][i];
                        actionInv->insert(add);
                    }
                }
                if (first) {
                    first = false;
                    accumState->insert(actionInv->begin(), actionInv->end());
                } else { // intersect
                    for (auto iter = accumState->begin(); iter != accumState->end();) {
                        int p = *iter;
                        if (actionInv->find(p) == actionInv->end()) {
                            iter = accumState->erase(iter);
                        } else {
                            ++iter;
                        }
                    }
                }
            }
            if (!accumState->empty()) {
                arcs.clear();
                for (ArcIterator<StdFst> aiter(*fst, state); !aiter.Done(); aiter.Next()) {
                    const StdArc &arc = aiter.Value();
                    int action = arc.ilabel - 1;
                    bool applicable = true;
                    for (int i = 0; i < htn->numPrecs[action]; i++) {
                        int p = htn->precLists[action][i];
                        int var = htn->bitToVar[p];
                        for (int j = htn->firstIndex[var]; j <= htn->lastIndex[var]; j++) {
                            if ((j != p) && (accumState->find(j) != accumState->end())) {
                                applicable = false;
                                break;
                            }
                        }
                        if (!applicable) {
                            break;
                        }
                    }
                    if (applicable) {
                        arcs.push_back(arc.ilabel - 1);
                        arcs.push_back(arc.nextstate);
                    }
                }
                fst->DeleteArcs(state);
                for (int i = 0; i < arcs.size(); i+=2) {
                    addRule(fst, state, arcs[i], arcs[i + 1], 0);
                }
            }
        }
    }
//    pFa->delta->ensureBW(); // label -> (to -> from)
//    unordered_map<tStateID, set<int>*> pStates; // partial state that holds with certainty
//    for (auto l : *pFa->delta->backward) {
//        if(l.first == epsilon)
//            continue;
//        for (auto to : *l.second) {
//            if (pStates.find(to.first) == pStates.end()) {
//                set<int>* accumState = new set<int>;
//                pStates.insert({to.first, accumState});
//                int action = l.first;
//
//                // preconditions that are not changed
//                for (int i = 0; i < htn->numPrecs[action]; i++) {
//                    int prec = htn->precLists[action][i];
//                    accumState->insert(prec);
//                }
//                // effects
//                for (int i = 0; i < htn->numDels[action]; i++) {
//                    int del = htn->delLists[action][i];
//                    accumState->erase(del);
//                }
//                for (int i = 0; i < htn->numAdds[action]; i++) {
//                    int add = htn->addLists[action][i];
//                    accumState->insert(add);
//                }
//            } else {
//                set<int>* accumState = pStates.find(to.first)->second;
//                if (accumState == nullptr) {
//                    continue;
//                }
//                // intersect old set with (unchanged precs \cup add effects)
//                set<int>* tempPState = new set<int>;
//                int action = l.first;
//
//                // preconditions that are not changed
//                for (int i = 0; i < htn->numPrecs[action]; i++) {
//                    int prec = htn->precLists[action][i];
//                    tempPState->insert(prec);
//                }
//                // effects
//                for (int i = 0; i < htn->numDels[action]; i++) {
//                    int del = htn->delLists[action][i];
//                    tempPState->erase(del);
//                }
//                for (int i = 0; i < htn->numAdds[action]; i++) {
//                    int add = htn->addLists[action][i];
//                    tempPState->insert(add);
//                }
//                for(auto iter = accumState->begin(); iter != accumState->end(); ) {
//                    int p = *iter;
//                    if (tempPState->find(p) == tempPState->end()) {
//                        iter = accumState->erase(iter);
//                    } else {
//                        ++iter;
//                    }
//                }
//                delete tempPState;
//
//                if (accumState->empty()) { // closed
//                    delete accumState;
//                    pStates.at(to.first) = nullptr;
//                }
//            }
//        }
//    }
////    cout << endl;
////    for (auto accumState : pStates) {
////        if (accumState.second == nullptr) {
////            continue;
////        }
////        cout << accumState.first <<  ":";
////        for(int p : *accumState.second) {
////            cout << " " << htn->factStrs[p];
////        }
////         cout << endl;
////    }
////    cout << endl;
//
//    pFa->delta->ensureFW(); // from -> (label -> to)
//    for (auto iter2 = pFa->delta->forward->begin(); iter2 != pFa->delta->forward->end();) {
//        auto from = *iter2;
//        if (pFa->sInit.find(from.first) !=pFa->sInit.end()) {
//            iter2++;
//            continue;
//        }
//        if ((pStates.find(from.first) == pStates.end()) || (pStates.find(from.first)->second == nullptr)) {
//            iter2++;
//            continue;
//        }
//
//        set<int> accumState = *pStates.find(from.first)->second;
//
//        bool totallyEmpty = false;
//        for (auto iter = from.second->begin(); iter != from.second->end(); ) {
//            auto actions = *iter;
//            bool applicable = true;
//            int action = actions.first;
//            if(action == epsilon) {
//                iter++;
//                continue;
//            }
//            for (int i = 0; i < htn->numPrecs[action]; i++) {
//                int p = htn->precLists[action][i];
//                int var =  htn->bitToVar[p];
//                for (int j = htn->firstIndex[var]; j <= htn->lastIndex[var]; j++) {
//                    if ((j != p) && (accumState.find(j) != accumState.end())) {
//                        applicable = false;
//                        break;
//                    }
//                }
//                if (!applicable) {
//                    break;
//                }
//            }
//            if (!applicable) {
//                delete actions.second;
//                iter = from.second->erase(iter);
//                pFa->delta->numTransitions--;
//                if (from.second->empty()) {
//                    iter2 = pFa->delta->forward->erase(iter2);
//                    totallyEmpty = true;
//                    break;
//                }
//            } else {
//                iter++;
//            }
//        }
//        if (!totallyEmpty) {
//            iter2++;
//        }
//    }
//    for (auto accumState : pStates) {
//        delete accumState.second;
//    }
}

void CFGtoFDAtranslator::addRule(StdVectorFst *fst, int from, int label, int to, int costs) {
    fst->AddArc(from, StdArc(label + 1, label + 1, 0, to));
}

int CFGtoFDAtranslator::nextState(StdVectorFst *fst) {
    fst->AddState();
    return fst->NumStates() - 1;
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

StdVectorFst *CFGtoFDAtranslator::getNewFA() {
    StdVectorFst *fst = new StdVectorFst();
    fst->SetProperties(kAcceptor, true);
    return fst;
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

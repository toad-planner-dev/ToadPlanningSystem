//
// Created by dh on 17.03.21.
//

#include "FA.h"
#include "NFAtoDFA/psState.h"
#include "../utils/IntPairHeap.h"
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;
using namespace progression;

FA::FA() {
    this->delta = new TransitionContainer;
}

FA::~FA() {
    delete this->delta;
}

//
// Hopcroft's algorithm
//
//#define debugOutput
void FA::minimize() {
    if ((numStates == 0) || (sInit.size() == 0) || (sGoal.size() == 0) || (numSymbols == -1)) {
        cout << "Error: Please properly initialize DFA before calling the minimization." << endl;
        exit(-1);
    }
    //assert(this->numStatesCorrect());
    int numP = 2; // current number of partitions

    partitions = new int[numStates]; // list of partByPartition elements
    for (int i = 0; i < numStates; i++) {
        partitions[i] = i;
    }

    // mapping of states to partitions
    s2p = new int[numStates];
    for (int i = 0; i < numStates; i++) {
        s2p[i] = 0;
    }
    for (int i : sGoal) {
        s2p[i] = 1;
    }

#ifdef debugOutput
    cout << endl;
    for (int i = 0; i < numStates; i++) {
        cout << partitions[i] << " " << s2p[i] << endl;
    }
#endif

    sortByPartition(0, numStates - 1);

#ifdef debugOutput
    cout << endl;
    for (int i = 0; i < numStates; i++) {
        cout << partitions[i] << " " << s2p[i] << endl;
    }
#endif

    // start and end index of each partByPartition in the "partitions" array
    firstI.clear();
    lastI.clear();
    firstI.push_back(0);
    lastI.push_back(numStates - 1 - sGoal.size()); // the last one is the final state
    firstI.push_back(numStates - sGoal.size());
    lastI.push_back(numStates - 1);

    set<int> W;
    W.insert(0); // this is a partition id
    W.insert(1);

    while (!W.empty()) {
        int A = *W.begin();
        W.erase(W.begin());

        // original A will be split, but its start and end index is stable
        int AStart = firstI[A];
        int AEnd = lastI[A];

        for (int c = 0; c < numSymbols; c++) { // iterate over symbols
            reachesAbyCtoX(AStart, AEnd, c); // fills X in the class
            if (X.empty()) {
                continue;
            }

#ifdef debugOutput
            cout << "A: ";
            for (int i = AStart; i <= AEnd; i++) {
                cout << partitions[i] << " ";
            }
            cout << "sym: " << c << endl;

            for (int i = 0; i < X.size(); i++)
                cout << X[i] << " ";
            cout << endl << endl;
#endif

            for (int Y = 0; Y < numP; Y++) {
                if (XYintersectNotEq(Y)) {
                    int newP = numP++;
                    for (int i : inRest) { // this has been set by the XYintersectNotEq method
                        assert(i >= firstI[Y]);
                        assert(i <= lastI[Y]);
                        s2p[i] = newP; // create new partByPartition
                    }

                    sortByPartition(firstI[Y], lastI[Y]); // divide partitions

#ifdef debugOutput
                    cout << endl;
                    for (int i = 0; i < numStates; i++) {
                        cout << i << " " << partitions[i] << " " << s2p[i] << endl;
                    }
#endif

                    // update indices
                    firstI.push_back(lastI[Y] - (inRest.size() - 1));
                    lastI.push_back(lastI[Y]);
                    lastI[Y] = firstI[newP] - 1;

#ifdef debugOutput
                    for (int i = 0; i < firstI.size(); i++) {
                        cout << "s" << i << " from " << firstI[i] << " to " << lastI[i] << endl;
                    }
#endif

                    // add new sets to W
                    if (W.find(Y) != W.end()) {
                        W.insert(newP);
                    } else if ((lastI[Y] - firstI[Y]) <= (lastI[newP] - firstI[newP])) {
                        W.insert(Y);
                    } else {
                        W.insert(newP);
                    }
                }
            }
        }
    }

#ifdef debugOutput
    for (int i = 0; i < numStates; i++) {
        cout << partitions[i] << " " << s2p[i] << endl;
    }
#endif

    sortByIndex(0, numStates - 1);

    // update initial state
    vector<int> temp;
    for (int i : sInit) {
        temp.push_back(s2p[i]);
    }
    sInit.clear();
    for (int i : temp) { sInit.insert(i); }

    // update goal
    temp.clear();
    for (int i : sGoal) {
        temp.push_back(s2p[i]);
    }
    sGoal.clear();
    for (int i : temp) { sGoal.insert(i); }

    this->numStates = numP; // update max state id

    delta->indexTransformation(s2p); // update transitions

    // clean up
    delete[] partitions;
    delete[] s2p;
    firstI.clear();
    lastI.clear();
    X.clear();
    inRest.clear();
}


void FA::reachesAbyCtoX(int AStart, int AEnd, int c) {
    X.clear();
    for (int i = AStart; i <= AEnd; i++) {
        int elem = partitions[i];
        set<tStateID> *from = delta->getFrom(elem, c);
        if (from != nullptr) {
            for (int s : *from) {
                X.insert(s);
            }
        }
    }
}

bool FA::XYintersectNotEq(int Y) {
    bool interNonEmpty = false;
    inRest.clear();
    int iY = firstI[Y];
    assert(isSorted(Y));
    auto iX = X.begin();
    while (true) {
        int x = *iX;
        int y = partitions[iY];
        if (x == y) {
            interNonEmpty = true;
            iX++;
            iY++;
        } else if (y < x) {
            inRest.push_back(iY);
            iY++;
        } else { // (y > x)
            iX++;
        }
        if (iX == X.end()) {
            while (iY <= lastI[Y]) {
                inRest.push_back(iY++);
            }
            break;
        } else if (iY > lastI[Y]) {
            break;
        }
    }
    return (interNonEmpty && !inRest.empty());
}

void FA::sortByPartition(int lo, int hi) {
    if (lo < hi) {
        int p = partByPartition(lo, hi);
        sortByPartition(lo, p);
        sortByPartition(p + 1, hi);
    }
}

int FA::partByPartition(int lo, int hi) {
    int pivot = ((hi + lo) / 2);
    int i = lo - 1;
    int j = hi + 1;
    while (true) {
        do {
            i++;
        } while (compByPartition(i, pivot) < 0);
        do {
            j--;
        } while (compByPartition(j, pivot) > 0);
        if (i >= j) {
            return j;
        }
        if (i == pivot) {
            pivot = j;
        } else if (j == pivot) {
            pivot = i;
        }
        qsSwap(i, j);
    }
}

void FA::qsSwap(int i, int j) {
    int temp = partitions[i];
    partitions[i] = partitions[j];
    partitions[j] = temp;

    temp = s2p[i];
    s2p[i] = s2p[j];
    s2p[j] = temp;
}

int FA::compByPartition(int i, int j) {
    if (s2p[i] != s2p[j]) {
        return s2p[i] - s2p[j];
    } else {
        return partitions[i] - partitions[j];
    }
}

void FA::sortByIndex(int lo, int hi) {
    if (lo < hi) {
        int p = partByIndex(lo, hi);
        sortByIndex(lo, p);
        sortByIndex(p + 1, hi);
    }
}

int FA::partByIndex(int lo, int hi) {
    int pivot = ((hi + lo) / 2);
    int i = lo - 1;
    int j = hi + 1;
    while (true) {
        do { i++; } while (compByIndex(i, pivot) < 0);
        do { j--; } while (compByIndex(j, pivot) > 0);
        if (i >= j) {
            return j;
        }
        if(i == pivot) {
            pivot = j;
        } else if(j == pivot) {
            pivot = i;
        }
        qsSwap(i, j);
    }
}

int FA::compByIndex(int i, int j) {
    return partitions[i] - partitions[j];
}

void FA::printDOT() {
    cout << endl << "digraph D {" << endl << endl;
    for (int i : sInit) {
        cout << "   n" << i << " [shape=diamond]" << endl;
    }
    for (int g : sGoal) {
        cout << "   n" << g << " [shape=box]" << endl;
    }

    delta->fullIterInit();
    tStateID from, to;
    tLabelID label;
    while (delta->fullIterNext(&from, &label, &to)) {
        cout << "   n" << from << " -> n" << to << " [label=c" << label << "]" << endl;
    }

    cout << endl << "}" << endl;
}

void FA::showDOT() {
    system("rm fa.dot");
//    system("rm fa.dot.pdf");

    ofstream myfile;
    myfile.open ("fa.dot");

    myfile << endl << "digraph D {" << endl << endl;
    for (int i : sInit) {
        myfile << "   n" << i << " [shape=diamond]" << endl;
    }
    for (int g : sGoal) {
        myfile << "   n" << g << " [shape=box]" << endl;
    }

    delta->fullIterInit();
    tStateID from, to;
    tLabelID label;
    while (delta->fullIterNext(&from, &label, &to)) {
        myfile << "   n" << from << " -> n" << to << " [label=c" << label << "c]" << endl;
    }

    myfile << endl << "}" << endl;
    myfile.close();
    system("xdot fa.dot &");
//    system("dot -Tpdf -O fa.dot");
//    system("okular fa.dot.pdf &");
}

void FA::compileToDFA() {
    TransitionContainer *delta2 = new TransitionContainer;
    unordered_set<psState *, psStatePointedObjHash, psStatePointedEq> states;

    set<tStateID> *tempS0 = new set<tStateID>;
    for (int i : sInit) { tempS0->insert(i); }
    getEpsilonClosure(tempS0);
    psState *s0 = new psState(tempS0);
    s0->id = 0;
    delete tempS0;

    set<tStateID> tempGoal;
    for (int i : sInit) {
        if (sGoal.find(i) != sGoal.end()) {
            tempGoal.insert(s0->id);
            break;
        }
    }
    states.insert(s0);

    list<psState *> queue;
    queue.push_back(s0);

    while (!queue.empty()) {
        psState *sSet = queue.front();
        queue.pop_front();

//        cout << "poped:";
//        for (int i = 0; i < sSet->length; i++)
//            cout << " " << sSet->elems[i];
//        cout << endl;

        map<tLabelID, set<tStateID> *> arcsOut;
        for (int i = 0; i < sSet->length; i++) {
            int s = sSet->elems[i]; // part state

            // determine outgoing arcs
            delta->outIterInit(s);

            tStateID sTo;
            tLabelID alpha;
            while (delta->outIterNext(&alpha, &sTo)) {
                if (arcsOut.find(alpha) == arcsOut.end()) {
                    arcsOut.insert({alpha, new set<tStateID>});
                }
                arcsOut[alpha]->insert(sTo);

//                cout << " -- " << alpha << " --> " << sTo << endl;
            }
        }
        for (auto arcOut : arcsOut) {
            getEpsilonClosure(arcOut.second);
            psState *ps = new psState(arcOut.second);
            auto temp = states.find(ps);
            if (temp == states.end()) {
                ps->id = states.size();
                for (int s : *arcOut.second) {
                    if (sGoal.find(s) != sGoal.end()) {
                        tempGoal.insert(ps->id);
                        break;
                    }
                }
                states.insert(ps);
                queue.push_back(ps);
                //cout << "added " << sSet->id << " via " << arcOut.first << " to " << ps->id << endl;
                delta2->addRule(sSet->id, arcOut.first, ps->id);
            } else { // already there
                delta2->addRule(sSet->id, arcOut.first, (*temp)->id);
//                cout << "added " << sSet->id << " via " << arcOut.first << " to " << (*temp)->id << endl;
            }
        }
    }
    this->numStates = states.size();

    this->sInit.clear();
    this->sInit.insert(0);

    this->sGoal.clear();
    for (int g : tempGoal) this->sGoal.insert(g);

    delete delta;
    delta = delta2;
}

void FA::getEpsilonClosure(set<tStateID> *pSet) {
    list<tStateID> todo;
    todo.insert(todo.end(), pSet->begin(), pSet->end());
    while (!todo.empty()) {
        tStateID s = todo.front();
        todo.pop_front();
        delta->outIterInit(s, epsilon);
        tStateID to;
        while (delta->outIterNext(&to)) {
            if (pSet->find(to) == pSet->end()) {
                todo.push_back(to);
                pSet->insert(to);
            }
        }
    }
}

void FA::addRule(int from, int label, int to) {
    this->delta->addRule(from, label, to);
}

void FA::printRules() {
    cout << "init: ";
    for (int i : sInit) {
        cout << " n" << i;
    }
    cout << endl;
    cout << "goal: ";
    for (int g : sGoal) {
        cout << " n" << g;
    }
    cout << endl;
    cout << endl;

    delta->fullIterInit();
    tStateID from, to;
    tLabelID label;
    while (delta->fullIterNext(&from, &label, &to)) {
        cout << " - n" << from << " -> n" << to << " label=c" << label << endl;
    }
}

void FA::addSubFA(tStateID from, FA *pFa, tStateID to) {
    assert(pFa->sInit.size() == 1);
    assert(pFa->sGoal.size() == 1);
    tStateID s0 = *pFa->sInit.begin();
    tStateID g = *pFa->sGoal.begin();
    map<tStateID, tStateID> iMapping;
    iMapping.insert({s0, from});
    iMapping.insert({g, to});

    pFa->delta->fullIterInit();
    tStateID from2, to2;
    tLabelID l;
    while (pFa->delta->fullIterNext(&from2, &l, &to2)) {
        if (iMapping.find(from2) == iMapping.end()) {
            iMapping.insert({from2, this->numStates++});
        }
        if (iMapping.find(to2) == iMapping.end()) {
            iMapping.insert({to2, this->numStates++});
        }
        this->addRule(iMapping[from2], l, iMapping[to2]);
    }
//    if(!this->numStatesCorrect()) {
//        pFa->showDOT();
//        this->showDOT();
//    }
//    assert(this->numStatesCorrect());
    //cout << pFa->sGoal.size() << endl;
}

void FA::showDOT(string *pString) {
    system("rm fa.dot");
    //system("rm fa.dot.pdf");

    ofstream myfile;
    myfile.open ("fa.dot");

    myfile << endl << "digraph D {" << endl << endl;

    for (int i : sInit) {
        myfile << "   n" << i << " [shape=diamond]" << endl;
    }
    for (int g : sGoal) {
        myfile << "   n" << g << " [shape=box]" << endl;
    }

    delta->fullIterInit();
    tStateID from, to;
    tLabelID label;
    while (delta->fullIterNext(&from, &label, &to)) {
        myfile << "   n" << from << " -> n" << to << " [label=\"" << pString[label] << "\"]" << endl;
    }

    myfile << endl << "}" << endl;
    myfile.close();
    system("xdot fa.dot");
    //system("dot -Tpdf -O fa.dot");
    //system("okular fa.dot.pdf &");
}

bool FA::numStatesCorrect() {
    set<int> states;
    delta->fullIterInit();
    tStateID from, to;
    tLabelID label;
    while (delta->fullIterNext(&from, &label, &to)) {
        states.insert(from);
        states.insert(to);
    }
    for (int s : sInit) {
        states.insert(s);
    }
    for (int s : sGoal) {
        states.insert(s);
    }
    for (int s : states) {
        if (s >= states.size()) {
            //cout << "size " << states.size() << endl;
//            for (int i = 0; i < states.size(); i++) {
//                if (states.find(i) == states.end()) {
//                    cout << "missing " << i << endl;
//                }
//            }
            return false;
        }
    }
    return (numStates == states.size());
}

bool FA::isPrimitive(int numTerminals) {
    delta->fullIterInit();
    tStateID from, to;
    tLabelID l;
    while (delta->fullIterNext(&from, &l, &to)) {
        if (l >= numTerminals) {
            return false;
        }
    }
    return true;
}

bool FA::isSorted(int y) {
    for (int i = firstI[y]; i < lastI[y]; i++) {
        if (partitions[i] > partitions[i + 1]) {
            return false;
        }
    }
    return true;
}

void FA::writeDfadHeuristic(string &file) {
    cout << "- init" << endl;
    int* hVals = new int[numStates];
    for(int i = 0; i < numStates; i++) {
        hVals[i] = INT_MAX;
    }

    IntPairHeap heap(1000);
    for(int g : sGoal) {
        heap.add(0, g); // final state with 0
    }

    cout << "- starting" << endl;
    while (!heap.isEmpty()) {
        int costs = heap.topKey();
        int state = heap.topVal();
        heap.pop();
        if (hVals[state] <= costs) {
            continue;
        }
        hVals[state] = costs;

        for (int c = 0; c < numSymbols; c++) {
            auto fromSet = delta->getFrom(state, c);
            if (fromSet == nullptr)
                continue;
            int aCosts = 1; // might be switched to action costs
            for (int from : *fromSet) {
                heap.add(costs + aCosts, from);
            }
        }
    }

    cout << "- done" << endl;
    ofstream os;
    os.open(file);
    os << numStates << "\n";
    for (int i = 0; i < numStates; i++) {
        os << hVals[i] << "\n";
    }
    os.close();
}

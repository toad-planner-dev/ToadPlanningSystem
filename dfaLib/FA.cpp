//
// Created by dh on 17.03.21.
//

#include "FA.h"
#include "NFAtoDFA/psState.h"

#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

FA::FA() {
    this->delta = new TransitionContainer;
}

FA::~FA() {
    delete this->delta;
}

//
// Hopcroft's algorithm
//
void FA::minimize() {
    if ((numStates == 0) || (sInit.size() == 0) || (sGoal.size() == 0) || (numSymbols == -1)) {
        cout << "Error: Please properly initialize DFA before calling the minimization." << endl;
        exit(-1);
    }

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

//    cout << endl;
//    for (int i = 0; i < numStates; i++) {
//        cout << partitions[i] << " " << s2p[i] << endl;
//    }
    sortByPartition(0, numStates - 1);

//    cout << endl;
//    for (int i = 0; i < numStates; i++) {
//        cout << partitions[i] << " " << s2p[i] << endl;
//    }

    // start and end index of each partByPartition in the "partitions" array
    firstI.clear();
    lastI.clear();
    firstI.push_back(0);
    lastI.push_back(numStates - 1 - sGoal.size()); // the last one is the final state
    firstI.push_back(numStates - sGoal.size());
    lastI.push_back(numStates - 1);

    list<int> W;
    W.push_back(0); // this is a partition id
    W.push_back(1);

    while (!W.empty()) {
        int A = W.front();
        W.pop_front();
        for (int c = 0; c < numSymbols; c++) { // iterate over symbols
//            cout << "A: ";
//            for (int i = firstI[A]; i <= lastI[A]; i++) {
//                cout << partitions[i] << " ";
//            }
//            cout << "sym: " << c << endl;
            reachesAbyCtoX(A, c); // fills X in the class
//            for (int i = 0; i < X.size(); i++)
//                cout << X[i] << " ";
//            cout << endl << endl;

            int oldSize = numP;
            for (int Y = 0; Y < oldSize; Y++) {
                if (XYintersectNotEq(Y)) {
                    int newP = numP++;
                    for (int i : inRest) { // this has been set by the XYintersectNotEq method
                        s2p[i] = newP; // create new partByPartition
                    }
                    sortByPartition(firstI[Y], lastI[Y]); // divide partitions

                    // update indices
                    firstI.push_back(lastI[Y] - (inRest.size() - 1));
                    lastI.push_back(lastI[Y]);
                    lastI[Y] = lastI[Y] - firstI[newP] - 1;

                    // add new sets to W
                    if (find(W.begin(), W.end(), Y) != W.end()) {
                        W.push_back(newP);
                    } else if ((lastI[Y] - firstI[Y]) <= (lastI[newP] - firstI[newP])) {
                        W.push_back(Y);
                    } else {
                        W.push_back(newP);
                    }
                }
            }
        }
    }
    sortByIndex(0, numStates - 1);

//    for (int i = 0; i < numStates; i++) {
//        cout << partitions[i] << " " << s2p[i] << endl;
//    }

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
    delta->updateFAData(s2p); // update transitions

    // clean up
    delete[] partitions;
    delete[] s2p;
    firstI.clear();
    lastI.clear();
    X.clear();
    inRest.clear();
}


void FA::reachesAbyCtoX(int A, int c) {
    X.clear();
    for (int sA = firstI[A]; sA <= lastI[A]; sA++) {
        int elem = partitions[sA];
        set<tStateID> *from = delta->getFrom(elem, c);
        if (from != nullptr) {
            for (int s : *from) {
                X.push_back(s);
            }
        }
    }

//    cout << "reachesAbyCtoX:" << endl;
//    for(int i = 0; i < X.size(); i++) {
//        cout << X[i] << " ";
//    }
//    cout << endl<< endl;
    sort(X.begin(), X.end());
    X.erase(unique(X.begin(), X.end()), X.end());

//    for(int i = 0; i < X.size(); i++) {
//        cout << X[i] << " ";
//    }
//    cout << endl<< endl;
}

bool FA::XYintersectNotEq(int Y) {
    bool interNonEmpty = false;
    inRest.clear();
    int iY = firstI[Y];
    int iX = 0;
    while (true) {
        int x = X[iX];
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
        if (iX >= X.size()) {
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
        do { i++; } while (compByPartition(i, pivot) < 0);
        do { j--; } while (compByPartition(j, pivot) > 0);
        if (i >= j) {
            return j;
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
    } else if (partitions[i] != partitions[j]) {
        return partitions[i] - partitions[j];
    } else {
        return 0;
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

void FA::compileToDFA() {
    TransitionContainer *delta2 = new TransitionContainer;

    int newStates = 1;
    unordered_set<psState *, psStatePointedObjHash, psStatePointedEq> states;
    tStateID *s0Set = new tStateID[sInit.size()];
    set<tStateID> tempGoal;
    int k = 0;
    for (int i : sInit) { s0Set[k++] = i; }

    psState *s0 = new psState(s0Set, sInit.size());
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
        cout << "poped:";
        for (int i = 0; i < sSet->length; i++)
            cout << " " << sSet->elems[i];
        cout << endl;

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
                cout << " -- " << alpha << " --> " << sTo << endl;
            }
        }
        for (auto arcOut : arcsOut) {
            tStateID *e = new tStateID[arcOut.second->size()];
            int i = 0;
            for (int s : *arcOut.second) {
                e[i++] = s;
            }
            psState *ps = new psState(e, arcOut.second->size());
            auto temp = states.find(ps);
            if (temp == states.end()) {
                ps->id = newStates++;
                for (int s : *arcOut.second) {
                    if (sGoal.find(s) != sGoal.end()) {
                        tempGoal.insert(ps->id);
                        break;
                    }
                }
                states.insert(ps);
                queue.push_back(ps);
                delta2->addRule(sSet->id, arcOut.first, ps->id);
                cout << "added " << sSet->id << " via " << arcOut.first << " to " << ps->id << endl;
            } else { // already there
                delta2->addRule(sSet->id, arcOut.first, (*temp)->id);
                cout << "added " << sSet->id << " via " << arcOut.first << " to " << (*temp)->id << endl;
            }
        }
    }
    this->numStates = newStates;
    delete delta;
    delta = delta2;
    this->sGoal.clear();
    for (int g : tempGoal) this->sGoal.insert(g);
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

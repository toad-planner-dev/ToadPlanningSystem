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
#include <cassert>

FA::FA() {
    this->data = new FAData;
}

FA::~FA() {
    for (auto toTuple : *data) {
        for (auto labelTuple : *toTuple.second) {
            delete labelTuple.second;
        }
        delete toTuple.second;
    }
    delete data;
}

//
// Hopcroft's algorithm
//
void FA::minimize() {
    if ((numStates == -1) || (sInit == -1) || (sGoal.size() == 0) || (numSymbols == -1)) {
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
                    for (int i : inRest) {
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

    // update information
    int numTransitions2 = 0;
    FAData *data2 = updateFAData(numTransitions2);
    this->numStates = numP;
    this->sInit = s2p[sInit];
    for (int &i : sGoal) {
        i = s2p[i];
    }
    sort(sGoal.begin(), sGoal.end());
    sGoal.erase(unique(sGoal.begin(), sGoal.end()), sGoal.end());
    delete this->data;
    this->data = data2;
    this->numTransitions = numTransitions2;

    // clean up
    delete[] partitions;
    delete[] s2p;
    firstI.clear();
    lastI.clear();
    X.clear();
    inRest.clear();
}

FAData *FA::updateFAData(int &numTransitions2) {
    FAData *data2 = new FAData;
    for (auto toTuple : *data) {
        int to = toTuple.first;
        int to2 = s2p[to];
        if (data2->find(to2) == data2->end()) {
            data2->insert({to2, new unordered_map<int, set<int> *>});
        }
        auto toSet2 = data2->at(to2);
        for (auto labelTuple : *toTuple.second) {
            int alpha = labelTuple.first;
            set<int> *fromSet;
            if (toSet2->find(alpha) == toSet2->end()) {
                fromSet = new set<int>;
                toSet2->insert({alpha, fromSet});
            } else {
                fromSet = toSet2->at(alpha);
            }
            for (int from : *labelTuple.second) {
                int from2 = s2p[from];
                if (fromSet->find(from2) == fromSet->end()) {
                    numTransitions2++;
                    fromSet->insert(from2);
                }
            }
            delete labelTuple.second;
        }
        delete toTuple.second;
    }
    return data2;
}

void FA::reachesAbyCtoX(int A, int c) {
    X.clear();
    for (int sA = firstI[A]; sA <= lastI[A]; sA++) {
        int elem = partitions[sA];
        set<int> *from = getFrom(elem, c);
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

set<int> *FA::getFrom(int to, int c) {
    if (data->find(to) != data->end()) {
        auto temp = data->at(to);
        if (temp->find(c) != temp->end()) {
            set<int> *res = temp->at(c);
            return res;
        }
    }
    return nullptr;
}

void FA::addRule(int from, int alpha, int to) {
    if ((alpha == -1) && (from == to)) // epsilon selfloop
        return;

    if (data->find(to) == data->end()) {
        data->insert({to, new unordered_map<int, set<int> *>});
    }

    set<int> *fromSet;
    if (data->at(to)->find(alpha) == data->at(to)->end()) {
        fromSet = new set<int>;
        data->at(to)->insert({alpha, fromSet});
    } else {
        fromSet = data->at(to)->at(alpha);
    }
    if (fromSet->find(from) == fromSet->end()) {
        this->numTransitions++;
        fromSet->insert(from);
    }
}

void FA::printDOT() {
    cout << endl << "digraph D {" << endl << endl;
    cout << "   n" << sInit << " [shape=diamond]" << endl;
    for (int g : sGoal) {
        cout << "   n" << g << " [shape=box]" << endl;
    }

    for (auto toTuple : *data) {
        int to = toTuple.first;
        for (auto labelTuple : *toTuple.second) {
            int alpha = labelTuple.first;
            for (int from : *labelTuple.second) {
                cout << "   n" << from << " -> n" << to << " [label=c" << alpha << "]" << endl;
            }
        }
    }
    cout << endl << "}" << endl;
}

void FA::compileToDFA() {
    int newStates = 0;
    unordered_set<psState*, psStatePointedObjHash, psStatePointedEq> states;
    psState *s0 = new psState(new int{sInit}, 1);
    if(sGoal.find(sInit) != sGoal.end()) {
        addTempGoal(s0->id);
    }
    states.insert(s0);

    list<psState*> queue;
    queue.push_back(s0);

    while (!queue.empty()) {
        psState* sSet = queue.front();
        queue.pop_front();

        map<int, set<int>*> arcsOut;
        for (int i = 0; i < sSet->length; i++) {
            int s = sSet->elems[i]; // part state

            // determine outgoing arcs
            int startOut;
            int endOut;
            getOutgoingArcs(s, &startOut, &endOut);
            for (int j = startOut; j <= endOut; j++) {
                int sFrom;
                int alpha;
                int sTo;
                getTransition(j, &sFrom, & alpha, & sTo);
                if (arcsOut.find(alpha) == arcsOut.end()) {
                    arcsOut.insert({alpha, new set<int>});
                }
                assert(s == sFrom);
                arcsOut[alpha]->insert(sTo);
            }
        }
        for(auto arcOut : arcsOut) {
            int* e = new int[arcOut.second->size()];
            int i= 0;
            for(int s : *arcOut.second) {
                e[i++] = s;
            }
            psState* ps = new psState(e, arcOut.second->size());
            auto temp = states.find(ps);
            if(temp == states.end()) {
                ps->id = newStates++;
                for(int s : *arcOut.second) {
                    if (sGoal.find(s) != sGoal.end()) {
                        addTempGoal(ps->id);
                        break;
                    }
                }
                states.insert(ps);
                queue.push_back(ps);
            } else { // already there
                addTempArc(sSet->id, arcOut.first, (*temp)->id);
            }
        }
    }
}

void FA::getOutgoingArcs(int s, int *startI, int *endI) {

}

void FA::getTransition(int j, int *pInt, int *pInt1, int *pInt2) {

}

void FA::addTempArc(int s1, const int alpha, int s2) {

}

void FA::addTempGoal(int s) {

}

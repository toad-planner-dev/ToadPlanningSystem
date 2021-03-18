//
// Created by dh on 17.03.21.
//

#include "FA.h"

#include <list>
#include <set>
#include <algorithm>
#include <iostream>

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

    partitions = new int[numStates]; // list of qsPart elements
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

    qSort(0, numStates - 1);

    // start and end index of each qsPart in the "partitions" array
    firstI.clear();
    lastI.clear();
    firstI.push_back(0);
    lastI.push_back(numStates - 2); // the last one is the final state
    firstI.push_back(numStates - 1);
    lastI.push_back(numStates - 1);

    list<int> W;
    W.push_back(0); // this is a partition id
    W.push_back(1);

    while (!W.empty()) {
        int A = W.front();
        W.pop_front();
        for (int c = 0; c < numSymbols; c++) { // iterate over symbols
            cout << "A: ";
            for (int i = firstI[A]; i <= lastI[A]; i++) {
                cout << partitions[i] << " ";
            }
            cout << "sym: " << c << endl;
            reachesAbyCtoX(A, c); // fills X in the class
            for (int i = 0; i < X.size(); i++)
                cout << X[i] << " ";
            cout << endl << endl;

            int oldSize = numP;
            for (int Y = 0; Y < oldSize; Y++) {
                if (XYintersectNotEq(Y)) {
                    int newP = numP++;
                    for (int i : inRest) {
                        s2p[i] = newP; // create new qsPart
                    }
                    qSort(firstI[Y], lastI[Y]); // divide partitions

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

    for (int i = 0; i < numStates; i++) {
        cout << partitions[i] << " " << s2p[i] << endl;
    }

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
    /*
    cout << "reachesAbyCtoX:" << endl;
    for(int i = 0; i < X.size(); i++) {
        cout << X[i] << " ";
    }
    cout << endl<< endl;*/
    sort(X.begin(), X.end());
    X.erase(unique(X.begin(), X.end()), X.end());
    /*
    for(int i = 0; i < X.size(); i++) {
        cout << X[i] << " ";
    }
    cout << endl<< endl;*/
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

void FA::qSort(int lo, int hi) {
    if (lo < hi) {
        int p = qsPart(lo, hi);
        qSort(lo, p);
        qSort(p + 1, hi);
    }
}

int FA::qsPart(int lo, int hi) {
    int pivot = ((hi + lo) / 2);
    int i = lo - 1;
    int j = hi + 1;
    while (true) {
        do { i++; } while (comp(i, pivot) < 0);
        do { j--; } while (comp(j, pivot) > 0);
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

int FA::comp(int i, int j) {
    if (s2p[i] != s2p[j]) {
        return s2p[i] - s2p[j];
    } else if (partitions[i] != partitions[j]) {
        return partitions[i] - partitions[j];
    } else {
        return 0;
    }
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

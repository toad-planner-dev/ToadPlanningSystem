//
// Created by dh on 17.03.21.
//

#ifndef TOAD_FA_H
#define TOAD_FA_H

#include <vector>
#include <set>
#include <unordered_map>

using namespace std;

typedef unordered_map<int, unordered_map<int, set<int> *> *> FAData;

class FA {
    unordered_map<int, unordered_map<int, set<int> *> *> *data;
    int numTransitions = 0;

    // for minimization
    int *partitions;
    int *s2p;
    vector<int> firstI;
    vector<int> lastI;
    vector<int> X;
    vector<int> inRest;


    void qSort(int i, int i1);

    int qsPart(int lo, int hi);

    void reachesAbyCtoX(int a, int c);

    bool XYintersectNotEq(int Y);

    int comp(int i, int pivot);

    FAData *updateFAData(int &numTransitions2);

public:
    FA();
    ~FA();
    int numStates = -1;
    int sInit = -1;
    vector<int> sGoal;
    int numSymbols = -1;

    void addRule(int from, int alpha, int to);

    void minimize();

    set<int> *getFrom(int to, int c);

    void qsSwap(int i, int j);
};


#endif //TOAD_FA_H

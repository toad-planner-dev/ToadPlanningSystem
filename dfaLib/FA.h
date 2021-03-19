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


    void sortByPartition(int i, int i1);

    int partByPartition(int lo, int hi);

    void reachesAbyCtoX(int a, int c);

    bool XYintersectNotEq(int Y);

    int compByPartition(int i, int pivot);

    FAData *updateFAData(int &numTransitions2);

public:
    FA();
    ~FA();
    int numStates = -1;
    int sInit = -1;
    set<int> sGoal;
    int numSymbols = -1;

    void addRule(int from, int alpha, int to);

    void minimize();

    void compileToDFA();

    set<int> *getFrom(int to, int c);

    void qsSwap(int i, int j);

    void printDOT();

    void sortByIndex(int lo, int hi);

    int partByIndex(int lo, int hi);

    int compByIndex(int i, int j);

    void getOutgoingArcs(int s, int *startI, int *endI);

    void getTransition(int j, int *pInt, int *pInt1, int *pInt2);

    void addTempArc(int s1, const int alpha, int s2);

    void addTempGoal(int s);
};


#endif //TOAD_FA_H

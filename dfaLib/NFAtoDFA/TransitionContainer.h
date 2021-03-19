//
// Created by dh on 19.03.21.
//

#ifndef TOAD_TRANSITIONCONTAINER_H
#define TOAD_TRANSITIONCONTAINER_H

#include <vector>

using namespace std;

typedef unsigned int fdaType;

class TransitionContainer {
    const int cFrom = 0;
    const int cLabel = 1;
    const int cTo = 2;

    const int containerMB = 100;
    const int containerSize = (containerMB * 1024 * 1024) / sizeof(fdaType) / 3;
    int size;
    vector<int*> data;

    int sortedBy = -1;

    void quicksort(int lo, int hi, int by);
    int partition(int lo, int hi, int by);
    int compare(int i, int j, int by);
public:
    TransitionContainer();
    ~TransitionContainer();
    void sortByFrom();
    void sortByLabel();
    void sortByTo();
    void makeSortedSet();

    void get(int i, int *from, int *alpha, int *to);
    void append(int from, int alpha, int to);
    void set(int i, int from, int alpha, int to);

};


#endif //TOAD_TRANSITIONCONTAINER_H

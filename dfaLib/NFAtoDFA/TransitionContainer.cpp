//
// Created by dh on 19.03.21.
//

#include <climits>
#include "TransitionContainer.h"

TransitionContainer::TransitionContainer() {
    this->size = 0;
    this->data.push_back(new int[containerSize * 3]);
}

TransitionContainer::~TransitionContainer() {
    for (int *d : data) {
        delete[] d;
    }
}

void TransitionContainer::makeSortedSet() {
    this->sortByFrom();
    int numDels = 0;
    for (int i = 0; i < (size - 1) * 3; i += 3) {
        const int blockIndex = i / (containerSize * 3);
        const int cellIndex = i % (containerSize * 3);
        if ((data[blockIndex][cellIndex + 0] == data[blockIndex][cellIndex + 3])
            && (data[blockIndex][cellIndex + 1] == data[blockIndex][cellIndex + 4])
            && (data[blockIndex][cellIndex + 2] == data[blockIndex][cellIndex + 5])) {
            data[blockIndex][cellIndex + 0] = UINT_MAX;
            data[blockIndex][cellIndex + 1] = UINT_MAX;
            data[blockIndex][cellIndex + 2] = UINT_MAX;
            numDels++;
        }
    }

    if (sortedBy >= 0) {
        const int store = sortedBy;
        sortedBy = -1;
        quicksort(0, size - 1, store);
    }
    size -= numDels; // after sorting, these will be at the end
}

void TransitionContainer::get(int i, int *from, int *alpha, int *to) {
    // transition 0 is in the cells 0, 1, 2; transition 1 in 3, 4, 5, and so on
    const int blockIndex = i / containerSize;
    const int cellIndex = (i % containerSize) * 3;
    *from = data[blockIndex][cellIndex];
    *alpha = data[blockIndex][cellIndex + 1];
    *to = data[blockIndex][cellIndex + 2];
}

void TransitionContainer::append(int from, int alpha, int to) {
    sortedBy = -1;
    const int blockIndex = size / containerSize;
    const int cellIndex = (size % containerSize) * 3;
    data[blockIndex][cellIndex] = from;
    data[blockIndex][cellIndex + 1] = alpha;
    data[blockIndex][cellIndex + 2] = to;
    size++;
    if (size >= (containerSize * data.size())) {
        this->data.push_back(new int[containerSize * 3]);
    }
}

void TransitionContainer::set(int i, int from, int alpha, int to) {
    sortedBy = -1;
    const int blockIndex = i / containerSize;
    const int cellIndex = (i % containerSize) * 3;
    data[blockIndex][cellIndex] = from;
    data[blockIndex][cellIndex + 1] = alpha;
    data[blockIndex][cellIndex + 2] = to;
}

void TransitionContainer::sortByFrom() {
    if (sortedBy != cFrom) {
        quicksort(0, size - 1, cFrom);
        sortedBy = cFrom;
    }
}

void TransitionContainer::sortByLabel() {
    if (sortedBy != cLabel) {
        quicksort(0, size - 1, cLabel);
        sortedBy = cLabel;
    }
}

void TransitionContainer::sortByTo() {
    if (sortedBy != cTo) {
        quicksort(0, size - 1, cTo);
        sortedBy = cTo;
    }
}

void TransitionContainer::quicksort(int lo, int hi, int by) {
    if (lo < hi) {
        int p = partition(lo, hi, by);
        quicksort(lo, p - 1, by);
        quicksort(p + 1, hi, by);
    }
}

int TransitionContainer::partition(int lo, int hi, int by) {
    int pivot = (hi + lo) / 2;
    int i = lo - 1;
    int j = hi + 1;
    while (true) {
        do {
            i++;
        } while (compare(i, pivot, by) < 0);
        do {
            j--;
        } while (compare(j, pivot, by) > 0);
        if (i >= j) {
            return j;
        }
        // swap A[i] with A[j]
        int iFrom, iLabel, iTo;
        int jFrom, jLabel, jTo;
        get(i, &iFrom, &iLabel, &iTo);
        get(j, &jFrom, &jLabel, &jTo);
        set(j, iFrom, iLabel, iTo);
        set(i, jFrom, jLabel, jTo);
    }
}

int TransitionContainer::compare(int i, int j, int by) {
    const int blockIndexI = i / containerSize;
    const int cellIndexI = (i % containerSize) * 3 + by;
    const int blockIndexJ = j / containerSize;
    const int cellIndexJ = (j % containerSize) * 3 + by;
    const int vi = data[blockIndexI][cellIndexI];
    const int vj = data[blockIndexJ][cellIndexJ];
    return vi - vj;
}

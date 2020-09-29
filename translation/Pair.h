//
// Created by dh on 29.09.20.
//

#ifndef TOSTRIPSAPPROXIMATION_PAIR_H
#define TOSTRIPSAPPROXIMATION_PAIR_H

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <iostream>

using namespace std;


class Pair {
public:
    Pair(int f, int t) {
        this->from = f;
        this->to = t;
    }

    int from;
    int to;
};


struct pairComp {
    bool operator()(const Pair *pa, const Pair *pb) const {
        return (pa->from == pb->from) && (pa->to == pb->to);
    }
};

class pairHash {
public:
    size_t operator()(const Pair *t) const {
        return ((t->from * 73856093u) ^ (t->to * 19349669u));
    }
};


#endif //TOSTRIPSAPPROXIMATION_PAIR_H

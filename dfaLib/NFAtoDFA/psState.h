//
// Created by dh on 19.03.21.
//

#ifndef TOAD_PSSTATE_H
#define TOAD_PSSTATE_H

#include <algorithm>
#include <functional>
#include "../TransitionContainer.h"

using namespace std;

inline void hash_combine(std::size_t &seed) {}

template<typename T, typename... Rest>
inline void hash_combine(std::size_t &seed, const T &v, Rest... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
}

struct psState {
    tStateID id;
    tStateID *elems;
    int length;
    size_t hash = -1;

    psState(){}

    psState(set<tStateID>* e){
        this->elems = new tStateID[e->size()];
        int i = 0;
        for (int el : *e) {
            this->elems[i++] = el;
        }
        this->length = e->size();
    }

    psState(tStateID* e, int length) {
        elems = e;
        this->length = length;
    }
};

struct psStatePointedObjHash {
    size_t operator()(psState *s) const {
        if(s->hash == -1) {
            s->hash = 1;
            for (int i = 0; i < s->length; i++) {
                hash_combine(s->hash, s->elems[i]);
            }
        }
        return s->hash;
    }
};

struct psStatePointedEq {
    bool operator()(psState const *s1, psState const *s2) const {
        if (s1->length != s2->length) {
            return false;
        } else {
            for (int i = 0; i < s1->length; i++) {
                if (s1->elems[i] != s2->elems[i]) {
                    return false;
                }
            }
            return true;
        }
    }
};

#endif //TOAD_PSSTATE_H

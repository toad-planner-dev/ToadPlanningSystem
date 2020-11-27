//
// Created by dh on 26.11.20.
//

#ifndef TOAD_MACROACTION_H
#define TOAD_MACROACTION_H

#include <vector>
#include <map>
using namespace std;

class macroAction {

public:
    macroAction(int i);
    virtual ~macroAction();

    void add(int i);

    int sfrom = -2;
    vector<int> *aSeq;
    int sTo = -2;
    map<int, int> *precs;
    map<int, int> *effects;
    bool isBroken = false;
};


#endif //TOAD_MACROACTION_H

//
// Created by dh on 17.03.21.
//

#include "FA.h"
#include "NFAtoDFA/psState.h"
#include "../utils/IntPairHeap.h"
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sys/time.h>

using namespace std;
using namespace progression;

FA::FA() {
    this->fst = new StdVectorFst();
    fst->SetProperties(kAcceptor, true);
}

FA::~FA() {

}

void FA::minimize() {
    Minimize(fst);
}

void FA::addRule(int from, int label, int to) {
    fst->AddArc(from, StdArc(label + 1, label + 1, 0, to));
}

//void FA::printRules() {
//    cout << "init: ";
//    for (int i : sInit) {
//        cout << " n" << i;
//    }
//    cout << endl;
//    cout << "goal: ";
//    for (int g : sGoal) {
//        cout << " n" << g;
//    }
//    cout << endl;
//    cout << endl;
//
////    delta->fullIterInit();
////    tStateID from, to;
////    tLabelID label;
////    while (delta->fullIterNext(&from, &label, &to)) {
////        cout << " - n" << from << " -> n" << to << " label=c" << label << endl;
////    }
//}

void FA::addSubFA(tStateID from, FA *pFa, tStateID to) {

}

void FA::showDOT(string *pString) {
    ofstream myfile;
    myfile.open("syms.txt");
    myfile << "<eps> 0\n";
    for(int i = 0; i < this->numSymbols; i++) {
        myfile << pString[i] << " " << (i + 1) << "\n";
    }
    myfile.close();

    system("rm binary.fst");
    fst->Write("binary.fst");
    system("fstdraw --acceptor=true --isymbols=syms.txt --osymbols=syms.txt binary.fst binary.dot");
    system("xdot binary.dot &");
}

void FA::showDOT() {
    system("rm binary.fst");
    fst->Write("binary.fst");
    system("fstdraw binary.fst binary.dot"); //  --acceptor=true
    system("xdot binary.dot &");
}

bool FA::numStatesCorrect() {
    return true;
}

bool FA::isPrimitive(int numTerminals) {
    return true;
}

//void FA::printDOT() {
//
//}

int FA::nextState() {
    fst->AddState();
    return fst->NumStates() - 1;
}

int FA::getNumStates() {
    return fst->NumStates();
}

void FA::makeInit(int state) {
    fst->SetStart(state);
}

void FA::makeFinal(int state) {
    fst->SetFinal(state);
}

//void FA::writeDfadHeuristic(string &file) {
//    int* hVals = new int[numStates];
//    for(int i = 0; i < numStates; i++) {
//        hVals[i] = -1; // todo: unreachable parts should be make expensive
//    }
//
//    IntPairHeap heap(1000);
//    for(int g : sGoal) {
//        heap.add(0, g); // final state with 0
//        hVals[g] = 0;
//    }
//
//    timeval tp;
//    gettimeofday(&tp, NULL);
//    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//
//    unordered_map<tStateID, unordered_set<tStateID> *>* bw = new unordered_map<tStateID, unordered_set<tStateID> *>;
//    delta->fullIterInit();
//    tStateID from, to;
//    tLabelID l;
//    while (delta->fullIterNext(&from, &l, &to)) {
//        unordered_set<tStateID> * fromSet;
//        auto elem = bw->find(to);
//        if (elem == bw->end()) {
//            fromSet = new unordered_set<tStateID>;
//            bw->insert({to, fromSet});
//        } else {
//            fromSet = elem->second;
//        }
//        fromSet->insert(from);
//    }
//
//    while (!heap.isEmpty()) {
//        int costs = heap.topKey();
//        int state = heap.topVal();
//        heap.pop();
//
//        auto elem = bw->find(state);
//        if (elem != bw->end()) {
//            auto fromSet = elem->second;
//            for (int from : *fromSet) {
//                if (hVals[from] < 0) {
//                    hVals[from] = costs + 1;
//                    heap.add(costs + 1, from);
//                }
//            }
//        }
//    }
//    gettimeofday(&tp, NULL);
//    long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
//    cout << " [timePrepareHdfad=" << (endT - startT) << "]" << endl;
//
//    ofstream os;
//    os.open(file);
//    os << numStates << "\n";
//    for (int i = 0; i < numStates; i++) {
//        if(hVals[i] >= 0)
//            os << hVals[i] << "\n";
//        else
//            os << 100000 << "\n";
//    }
//    os.close();
//}

//
// Created by dh on 03.10.20.
//

#include "ModelWriter.h"
#include <iostream>
#include <fst/vector-fst.h>

using namespace std;
using namespace fst;

void ModelWriter::write(Model *htn, fst::VectorFst <StdArc> *automaton, string dName, string pName) {
    this->m = htn;
    this->dfa = automaton;

    // check number of final states
    for (int i = 0; i < dfa->NumStates(); i++) {
        if (dfa->Final(i) == 0) {
            dfaGoalStates.push_back(i);
        }
    }
    if (dfaGoalStates.size() == 0) {
        cout << "ERROR: DFA has no goal state." << endl;
        exit(-1);
    }

    multipleGoals = (dfaGoalStates.size() > 1);
    if (multipleGoals) {
        finalState = dfa->NumStates();
    } else {
        finalState = dfaGoalStates[0];
    }

    ofstream dfile;
    cout << "- writing domain file" << endl;
    dfile.open(dName);
    writeDomain(dfile);
    dfile.close();
    cout << "- done" << endl;

    cout << "- writing domain file" << endl;
    ofstream pfile;
    pfile.open(pName);
    writeProblem(pfile, 0, 1);
    pfile.close();
    cout << "- done" << endl;
}

void ModelWriter::writeDomain(ostream &os) {
    os << "(define (domain htn)\n";

    cout << "  - writing predicate definition" << endl;

    writePredDef(os, dfa->NumStates());

    cout << "  - writing action definition" << endl;
//    auto *extraStuff = dfa->getActionMap();
//    int tenpercent = m->numActions / 10;
//    int since = 0;
    for (StateIterator<StdVectorFst> siter(*dfa); !siter.Done(); siter.Next()) {
        int from = siter.Value();
        for (ArcIterator<StdFst> aiter(*dfa, from); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            const int to = arc.nextstate;
            const int a = arc.ilabel - 1;
            writeAction(os, a, from, to, from);
        }
    }
    if (multipleGoals) { // compile multiple goal values away
        for (int goal: dfaGoalStates) {
            writeEpsilonAction(os, goal, finalState, goal);
        }
    }

//    for (int i = 0; i < m->numActions; i++) {
//        if (extraStuff->find(i) == extraStuff->end()) {
//            cout << "- automaton contains no rule for action " << m->taskNames[i] << endl;
//            continue;
//        }
//        for (auto extra : *extraStuff->at(i)) {
//            int from = extra.first;
//            for (int to : *extra.second) {
//                writeAction(os, i, from, to, from);
//            }
//        }
//        if(++since > tenpercent) {
//            cout << "    " << i << "/" << m->numActions << endl;
//            since = 0;
//        }
//        //writeActionCF(os, i, extraStuff[i]);
//    }
//    //writeEpsilonActionCF(os, extraStuff[-1]);
//    if (extraStuff->find(-1) != extraStuff->end()) {
//        for (auto extra : *extraStuff->at(-1)) {
//            int from = extra.first;
//            for (int to : *extra.second) {
//                writeEpsilonAction(os, from, to, from);
//            }
//        }
//    } else {
//        cout << "- automaton contains no epsilon transitions." << endl;
//    }
    os << ")\n";
}

bool ModelWriter::getSASVal(int *store, int *somelist, int length) const {
    bool result = false;
    for (int j = 0; j < length; j++) {
        int val = somelist[j];
        int var = 0;
        while (!((m->firstIndex[var] <= val) && (m->lastIndex[var] >= val)))
            var++;
        if (store[var] != -1) {
            result = true;
        }
        store[var] = val - m->firstIndex[var];
    }
    return result;
}

void ModelWriter::writePredDef(ostream &os, int maxState) {
    os << "  (:types dfastate)\n";
    os << "  (:constants ";
    for (int i = 0; i < maxState; i++) {
        os << "s" << i << " ";
        if ((i % 10) == 9) {
            if (i < maxState - 1) {
                os << "- dfastate\n" << "              ";
            }
        }
    }
    os << " - dfastate\n";
    os << "\n" << "  ) \n\n";

    os << "  (:predicates ";
    for (int i = 0; i < m->numStateBits; i++) {
        if (m->factStrs[i] == "none-of-them") {
            os << "(none" << i << ")\n";
        } else {
            os << "(" << su.cleanStr(m->factStrs[i]) << ")\n";
        }
        os << "               ";
    }
    os << "(dfa ?s - dfastate)\n";
    os << "  ) \n\n";
}

void ModelWriter::writeAction(ostream &os, int action, int dfaPrec, int dfaAdd, int dfaDel) {
    os << "   (:action " << su.cleanStr(m->taskNames[action]) << "-id" << action << "\n";
    os << "    :precondition (and\n";
    for (int i = 0; i < m->numPrecs[action]; i++) {
        int prec = m->precLists[action][i];
        if (m->factStrs[prec] == "none-of-them" ) {
            os << "      (none" << prec << ")\n";
        } else {
            os << "      (" << su.cleanStr(m->factStrs[prec]) << ")\n";
        }
    }
    os << "      (dfa s" << dfaPrec << ")";
    os << ")\n";
    os << "    :effect (and\n";
    for (int i = 0; i < m->numAdds[action]; i++) {
        int add = m->addLists[action][i];
        if (m->factStrs[add] == "none-of-them" ) {
            os << "      (none" << add << ")\n";
        } else {
            os << "      (" << su.cleanStr(m->factStrs[add]) << ")\n";
        }
    }
    os << "      (dfa s" << dfaAdd << ")\n";
    for (int i = 0; i < m->numDels[action]; i++) {
        int del = m->delLists[action][i];
        if (m->factStrs[del] == "none-of-them" ) {
            os << "      (none" << del << ")\n";
        } else {
            os << "      (not (" << su.cleanStr(m->factStrs[del]) << "))\n";
        }
    }
    os << "      (not (dfa s" << dfaDel << ")))\n";
    os << "   ) \n\n";
}

void ModelWriter::writeEpsilonAction(ostream &os, int prec, int add, int del) {
    os << "   (:action epsilon-" << epsilonAcs++ << "\n";
    os << "    :precondition (and\n";
    os << "      (dfa s" << prec << "))\n";
    os << "    :effect (and\n";
    os << "      (dfa s" << add << ")\n";
    os << "      (not (dfa s" << del << ")))\n";
    os << "   ) \n\n";
}

void ModelWriter::writeProblem(ostream &os, int startState, int goalState) {
    os << "(define (problem p)\n";
    os << "   (:domain htn)\n";
    os << "   (:init\n";
    for (int i = 0; i < m->s0Size; i++) {
        if (m->factStrs[m->s0List[i]] == "none-of-them" ) {
            os << "      (none" << m->s0List[i] << ")\n";
        } else {
            os << "      (" << su.cleanStr(m->factStrs[m->s0List[i]]) << ")\n";
        }
    }
    os << "      (dfa s" << dfa->Start() << ")\n";
    os << "   )\n";
    os << "   (:goal (and\n";
    for (int i = 0; i < m->gSize; i++) {
        if (m->factStrs[m->gList[i]] == "none-of-them" ) {
            os << "      (none" << m->gList[i] << ")\n";
        } else {
            os << "      (" << su.cleanStr(m->factStrs[m->gList[i]]) << ")\n";
        }
    }
    os << "      (dfa s" << finalState << ")\n";
    os << "   ))\n";
    os << ")\n";
}

void ModelWriter::writeActionCF(ostream &os, int action, set<pair<int, int> *> *cfSet) {
    os << "   (:action " << m->taskNames[action] << "\n";
    os << "    :precondition (and\n";
    for (int i = 0; i < m->numPrecs[action]; i++) {
        int prec = m->precLists[action][i];
        os << "      (" << su.cleanStr(m->factStrs[prec]) << ")\n";
    }
    os << "    )\n";
    os << "    :effect (and\n";
    for (int i = 0; i < m->numAdds[action]; i++) {
        int add = m->addLists[action][i];
        os << "      (" << su.cleanStr(m->factStrs[add]) << ")\n";
    }
    for (int i = 0; i < m->numDels[action]; i++) {
        int del = m->delLists[action][i];
        os << "      (not (" << su.cleanStr(m->factStrs[del]) << "))\n";
    }
    for (pair<int, int> *extra : *cfSet) {
        os << "      (when (and (dfa s" << extra->first << "))";
        os << " (and (dfa s" << extra->second << ")";
        os << " (not (dfa s" << extra->first << "))))\n";
    }
    os << "    )\n";
    os << "   ) \n\n";
}

void ModelWriter::writeEpsilonActionCF(ostream &os, set<pair<int, int> *> *cfSet) {
    os << "   (:action epsilon-" << epsilonAcs++ << "\n";
    os << "    :precondition (and )\n";
    os << "    :effect (and\n";
    for (pair<int, int> *extra : *cfSet) {
        os << "      (when (and (dfa s" << extra->first << "))";
        os << " (and (dfa s" << extra->second << ")";
        os << " (not (dfa s" << extra->first << "))))\n";
    }
    os << "    )\n";
    os << "   ) \n\n";
}

string ModelWriter::sasCleanStr(string s) {
    if(s == "none-of-them") {
        return "<none of those>";
    }
    return s;
}

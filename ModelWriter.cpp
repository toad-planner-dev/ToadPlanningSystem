//
// Created by dh on 03.10.20.
//

#include "ModelWriter.h"
#include <iostream>
#include <fstream>

using namespace std;

void ModelWriter::write(Model *htn, FiniteAutomaton *automaton, string dName, string pName) {
    this->m = htn;
    this->dfa = automaton;

    ofstream dfile;
    dfile.open (dName);
    writeDomain(dfile);
    dfile.close();

    ofstream pfile;
    pfile.open (pName);
    writeProblem(pfile, 0, 1);
    pfile.close();
}

void ModelWriter::writeDomain(ostream &os) {
    cout << "- preparing sets of extra precs/effs...";
    unordered_map<int, set<pair<int, int>*>*> extraStuff;// = new unordered_map<int, set<int>*>;

    for (auto &it: this->dfa->fda) {
        Pair *p = it.first;
        set<int> *labels = it.second;
        for (int l : *labels) {
            if (extraStuff.find(l) == extraStuff.end()) {
                set<pair<int, int>*>* s = new set<pair<int, int>*>;
                extraStuff[l] = s;
            }
            extraStuff[l]->insert(new pair<int, int>(p->from, p->to));
        }
    }
    cout << "done" << endl;

    /*
     * write domain
     */
    os << "(define (domain htn)" << endl;

    writePredDef(os, dfa->stateID);

    for (int i = 0; i < m->numActions; i++) {
        if (extraStuff.find(i) == extraStuff.end())
            continue; // unreachable via automaton
        for (pair<int, int>* extra : *extraStuff[i]) {
            writeAction(os, i, extra->first, extra->second, extra->first);
        }
    }
    for (pair<int, int>* extra : *extraStuff[-1]) {
        writeEpsilonAction(os, extra->first, extra->second, extra->first);
    }
    os << ")" << endl;
}

void ModelWriter::writePredDef(ostream& os, int maxState) {
    os << "  (:types dfastate)" << endl;
    os << "  (:constants ";
    for (int i = 0; i < maxState; i++) {
        os << "s" << i << " ";
        if ((i % 10) == 9) {
            if (i < maxState - 1) {
                os << "- dfastate" << endl << "              ";
            }
        }
    }
    os << " - dfastate" << endl;
    os << endl << "  )" << endl << endl;

    os << "  (:predicates ";
    for (int i = 0; i < m->numStateBits; i++) {
        os << "(" << su.cleanStr(m->factStrs[i]) << ")" << endl;
        os << "               ";
    }
    os << "(dfa ?s - dfastate)" << endl;
    os << "  )" << endl << endl;
}

void ModelWriter::writeAction(ostream& os, int action, int dfaPrec, int dfaAdd, int dfaDel) {
    os << "   (:action " << m->taskNames[action] << endl;
    os << "    :precondition (and" << endl;
    for (int i = 0; i < m->numPrecs[action]; i++) {
        int prec = m->precLists[action][i];
        os << "      (" << su.cleanStr(m->factStrs[prec]) << ")" << endl;
    }
    os << "      (dfa s" << dfaPrec << "))" << endl;
    os << "    :effect (and" << endl;
    for (int i = 0; i < m->numAdds[action]; i++) {
        int add = m->addLists[action][i];
        os << "      (" << su.cleanStr(m->factStrs[add]) << ")" << endl;
    }
    os << "      (dfa s" << dfaAdd << ")" << endl;
    for (int i = 0; i < m->numDels[action]; i++) {
        int del = m->delLists[action][i];
        os << "      (not (" << su.cleanStr(m->factStrs[del]) << "))" << endl;
    }
    os << "      (not (dfa s" << dfaDel << ")))" << endl;
    os << "   )" << endl << endl;
}

void ModelWriter::writeEpsilonAction(ostream& os, int prec, int add, int del) {
    os << "   (:action epsilon-" << epsilonAcs++ << endl;
    os << "    :precondition (and" << endl;
    os << "      (dfa s" << prec << "))" << endl;
    os << "    :effect (and" << endl;
    os << "      (dfa s" << add << ")" << endl;
    os << "      (not (dfa s" << del << ")))" << endl;
    os << "   )" << endl << endl;
}

void ModelWriter::writeProblem(ostream& os, int startState,int goalState) {
    os << "(define (problem p)" << endl;
    os << "   (:domain htn)" << endl;
    os << "   (:init" << endl;
    for (int i = 0; i < m->s0Size; i++) {
        os << "      (" << su.cleanStr(m->factStrs[m->s0List[i]]) << ")" << endl;
    }
    os << "      (dfa s" << dfa->startState << ")" << endl;
    os << "   )" << endl;
    os << "   (:goal (and" << endl;
    for (int i = 0; i < m->gSize; i++) {
        os << "      (" << su.cleanStr(m->factStrs[m->gList[i]]) << ")" << endl;
    }
    os << "      (dfa s" << dfa->finalState << ")" << endl;
    os << "   ))" << endl;
    os << ")" << endl;
}

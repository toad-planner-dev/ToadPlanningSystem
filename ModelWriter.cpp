//
// Created by dh on 03.10.20.
//

#include "ModelWriter.h"

ModelWriter::ModelWriter(Model *model, string domain, string problem) {
    this->m = model;
    this->dStr = domain;
    this->pStr = problem;

    cout << "(define (domain htn)" << endl;

}

void ModelWriter::writeAction(int action, int addP, int addAE, int addDE) {
    cout << "   (:action " << m->taskNames[action] << endl;
    cout << "    :precondition (and" << endl;
    for (int i = 0; i < m->numPrecs[action]; i++) {
        int prec = m->precLists[action][i];
        cout << "      (" << su.cleanStr(m->factStrs[prec]) << ")" << endl;
    }
    cout << "      (s" << addP << "))" << endl;
    cout << "    :effect (and" << endl;
    for (int i = 0; i < m->numAdds[action]; i++) {
        int add = m->addLists[action][i];
        cout << "      (" << su.cleanStr(m->factStrs[add]) << ")" << endl;
    }
    cout << "      (s" << addAE << ")" << endl;
    for (int i = 0; i < m->numDels[action]; i++) {
        int del = m->delLists[action][i];
        cout << "      (not (" << su.cleanStr(m->factStrs[del]) << "))" << endl;
    }
    cout << "      (not (s" << addDE << ")))" << endl;
    cout << "   )" << endl << endl;
}

void ModelWriter::writePredDef(int maxState) {
    cout << "  (:predicates ";
    for (int i = 0; i < m->numStateBits; i++) {
        cout << "(" << su.cleanStr(m->factStrs[i]) << ")" << endl;
        cout << "               ";
    }
    for (int i = 0; i < maxState; i++) {
        cout << "(s" << i << ")";
        if ((i % 10) == 9) {
            if (i < maxState - 1) {
                cout << endl << "               ";
            }
        }
    }
    cout << endl << "  )" << endl << endl;
}

void ModelWriter::writeProblem(int startState,int goalState) {
    cout << "(define (problem p)" << endl;
    cout << "   (:domain htn)" << endl;
    cout << "   (:init" << endl;
    for (int i = 0; i < m->s0Size; i++) {
        cout << "      (" << su.cleanStr(m->factStrs[m->s0List[i]]) << ")" << endl;
    }
    cout << "      (s" << startState << ")" << endl;
    cout << "   )" << endl;
    cout << "   (:goal (and" << endl;
    for (int i = 0; i < m->gSize; i++) {
        cout << "      (" << su.cleanStr(m->factStrs[m->gList[i]]) << ")" << endl;
    }
    cout << "      (s" << goalState << ")" << endl;
    cout << "   ))" << endl;
    cout << ")" << endl;
}

int epsilonAcs = 0;
void ModelWriter::writeEpsilonAction(int prec, int add, int del) {
    cout << "   (:action epsilon-" << epsilonAcs++ << endl;
    cout << "    :precondition (and" << endl;
    cout << "      (s" << prec << "))" << endl;
    cout << "    :effect (and" << endl;
    cout << "      (s" << add << ")" << endl;
    cout << "      (not (s" << del << ")))" << endl;
    cout << "   )" << endl << endl;
}

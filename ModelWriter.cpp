//
// Created by dh on 03.10.20.
//

#include "ModelWriter.h"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;

void ModelWriter::write(Model *htn, FiniteAutomaton *automaton, string dName, string pName) {
    this->m = htn;
    this->dfa = automaton;

    ofstream dfile;
    dfile.open(dName);
    writeDomain(dfile);
    dfile.close();

    ofstream pfile;
    pfile.open(pName);
    writeProblem(pfile, 0, 1);
    pfile.close();
}

void ModelWriter::writeDomain(ostream &os) {
    cout << "- preparing sets of extra precs/effs...";
    unordered_map<int, set<pair<int, int> *> *> extraStuff;

    for (auto &it: this->dfa->fda) {
        Pair *p = it.first;
        set<int> *labels = it.second;
        for (int l : *labels) {
            if (extraStuff.find(l) == extraStuff.end()) {
                set<pair<int, int> *> *s = new set<pair<int, int> *>;
                extraStuff[l] = s;
            }
            extraStuff[l]->insert(new pair<int, int>(p->from, p->to));
        }
    }
    cout << "done" << endl;

    /*
     * write domain
     */
    if (writePDDL) {
        os << "(define (domain htn)" << endl;

        writePredDef(os, dfa->stateID);

        for (int i = 0; i < m->numActions; i++) {
            if (extraStuff.find(i) == extraStuff.end())
                continue; // unreachable via automaton
            for (pair<int, int> *extra : *extraStuff[i]) {
                writeAction(os, i, extra->first, extra->second, extra->first);
            }
            //writeActionCF(os, i, extraStuff[i]);
        }
        //writeEpsilonActionCF(os, extraStuff[-1]);
        for (pair<int, int> *extra : *extraStuff[-1]) {
            writeEpsilonAction(os, extra->first, extra->second, extra->first);
        }
        os << ")" << endl;
    } else {
        os << "begin_version" << endl;
        os << "3" << endl;
        os << "end_version" << endl;
        os << "begin_metric" << endl;
        os << "0" << endl;
        os << "end_metric" << endl;

        os << m->numVars + 1 << endl; // number of variables

        // write original variables
        for (int i = 0; i < m->numVars; i++) {
            os << "begin_variable" << endl;
            os << "var" << i << endl;
            os << "-1" << endl;
            os << (m->lastIndex[i] - m->firstIndex[i] + 1) << endl;
            for (int j = m->firstIndex[i]; j <= m->lastIndex[i]; j++) {
                os << "Atom " << su.cleanStr(m->factStrs[j]) << endl;
            }
            os << "end_variable" << endl;
        }

        // write dfa states
        os << "begin_variable" << endl;
        os << "var" << m->numVars << endl;
        os << "-1" << endl;
        os << dfa->stateID << endl;
        for (int i = 0; i < dfa->stateID; i++) {
            os << "Atom dfa(s" << i << ")" << endl;
        }
        os << "end_variable" << endl;

        // mutex groups
        os << "0" << endl;

        // initial state
        os << "begin_state" << endl;
        int s0[m->numVars];
        for (int i = 0; i < m->numVars; i++) {
            s0[i] = -1;
        }
        for (int i = 0; i < m->s0Size; i++) {
            int val = m->s0List[i];
            int var = 0;
            while (!((m->firstIndex[var] <= val) && (m->lastIndex[var] >= val)))
                var++;
            if (s0[var] != -1) {
                cout << "error: two values of same sas+ variable are set in s0" << endl;
                exit(-1);
            }
            s0[var] = val - m->firstIndex[var];
        }
        for (int i = 0; i < m->numVars; i++) {
            os << s0[i] << endl;
        }
        os << "0" << endl; // initial value of automaton
        os << "end_state" << endl;

        // write goal definition
        os << "begin_goal" << endl;
        os << m->gSize + 1 << endl;
        for (int i = 0; i < m->gSize; i++) {
            int val = m->gList[i];
            int var = 0;
            while (!((m->firstIndex[var] <= val) && (m->lastIndex[var] >= val)))
                var++;
            os << var << " " << (val - m->firstIndex[var]);
        }
        os << m->numVars << " " << "1" << endl; // reach final state of automaton
        os << "end_goal" << endl;

        // count actions
        int numActions = 0;
        for (int i = -1; i < m->numActions; i++) {
            if (extraStuff.find(i) == extraStuff.end()) {
                continue; // unreachable via automaton
            }
            numActions += extraStuff[i]->size();
        }
        os << numActions << endl;

        // write actions
        int varPrec[m->numVars];
        int varAdd[m->numVars];
        int varDel[m->numVars];
        for (int i = 0; i < m->numVars; i++) {
            varPrec[i] = -1;
            varAdd[i] = -1;
            varDel[i] = -1;
        }
        vector<int> prevail;
        vector<int> effect;
        for (int i = 0; i < m->numActions; i++) {
            if (extraStuff.find(i) == extraStuff.end()) {
                continue; // unreachable via automaton
            }
            // generate FD's SAS+ format
            getSASVal(varPrec, m->precLists[i], m->numPrecs[i], i);
            getSASVal(varAdd, m->addLists[i], m->numAdds[i], i);
            getSASVal(varDel, m->delLists[i], m->numDels[i], i);
            prevail.clear();
            effect.clear();
            for (int j = 0; j < m->numVars; j++) {
                if ((varPrec[j] != -1) && (varAdd[j] == -1)) {
                    // prevail constraint
                    prevail.push_back(j);
                    prevail.push_back(varPrec[j]);
                    varPrec[j] = -1;
                } else if ((varPrec[j] != -1) && (varAdd[j] != -1)) {
                    if (varPrec[j] == varAdd[j]) { // this is actually a prevail constraint
                        prevail.push_back(j);
                        prevail.push_back(varPrec[j]);
                    } else {
                        effect.push_back(0); // not conditional
                        effect.push_back(j);
                        effect.push_back(varPrec[j]); // value needed before
                        effect.push_back(varAdd[j]); // value the variable is set to
                        assert(varDel[j] == varPrec[j]);
                    }
                    varPrec[j] = -1;
                    varAdd[j] = -1;
                    varDel[j] = -1;
                } else if ((varPrec[j] == -1) && (varAdd[j] != -1)) {
                    effect.push_back(0); // not conditional
                    effect.push_back(j);
                    effect.push_back(-1); // value needed before
                    effect.push_back(varAdd[j]); // value the variable is set to
                    varAdd[j] = -1;
                    cout << "!!!!!!!!!!!!!! It happens!" << endl;
                }
            }
            // write everything
            for (pair<int, int> *extra : *extraStuff[i]) {
                os << "begin_operator" << endl;
                os << m->taskNames[i] << endl;
                os << prevail.size() / 2 << endl;
                for (int j = 0; j < prevail.size(); j += 2) {
                    os << prevail[j] << " " << prevail[j + 1] << endl;
                }
                os << (effect.size() / 4 + 1) << endl; // one is added for the dfa
                for (int j = 0; j < effect.size(); j += 4) {
                    os << effect[j] << " " << effect[j + 1] << " " << effect[j + 2] << " " << effect[j + 3] << endl;
                }
                // dfa effect
                os << 0 << " " << m->numVars << " " << extra->first << " " << extra->second << endl;
                os << 0 << endl; // action costs, 0 means unicosts
                os << "end_operator" << endl;
            }
        }
        for (pair<int, int> *extra : *extraStuff[-1]) {
            os << "begin_operator" << endl;
            os << "epsilon" << endl;
            os << 0 << endl; // prevail constraints
            os << 1 << endl; // effects
            os << 0 << " " << m->numVars << " " << extra->first << " " << extra->second << endl;
            os << 0 << endl; // action costs, 0 means unicosts
            os << "end_operator" << endl;
        }
        os << 0 << endl;
    }
}

void ModelWriter::getSASVal(int *varPrec, int *l, int numVals, int action) const {
    for (int j = 0; j < numVals; j++) {
        int val = l[j];
        int var = 0;
        while (!((m->firstIndex[var] <= val) && (m->lastIndex[var] >= val)))
            var++;
        if (varPrec[var] != -1) {
            cout << "error: two values of same sas+ variable are in precondition/effect of action "
                 << m->taskNames[action] << endl;
            exit(-1);
        }
        varPrec[var] = val - m->firstIndex[var];
    }
}

void ModelWriter::writePredDef(ostream &os, int maxState) {
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

void ModelWriter::writeAction(ostream &os, int action, int dfaPrec, int dfaAdd, int dfaDel) {
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

void ModelWriter::writeEpsilonAction(ostream &os, int prec, int add, int del) {
    os << "   (:action epsilon-" << epsilonAcs++ << endl;
    os << "    :precondition (and" << endl;
    os << "      (dfa s" << prec << "))" << endl;
    os << "    :effect (and" << endl;
    os << "      (dfa s" << add << ")" << endl;
    os << "      (not (dfa s" << del << ")))" << endl;
    os << "   )" << endl << endl;
}

void ModelWriter::writeProblem(ostream &os, int startState, int goalState) {
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

void ModelWriter::writeActionCF(ostream &os, int action, set<pair<int, int> *> *cfSet) {
    os << "   (:action " << m->taskNames[action] << endl;
    os << "    :precondition (and" << endl;
    for (int i = 0; i < m->numPrecs[action]; i++) {
        int prec = m->precLists[action][i];
        os << "      (" << su.cleanStr(m->factStrs[prec]) << ")" << endl;
    }
    os << "    )" << endl;
    os << "    :effect (and" << endl;
    for (int i = 0; i < m->numAdds[action]; i++) {
        int add = m->addLists[action][i];
        os << "      (" << su.cleanStr(m->factStrs[add]) << ")" << endl;
    }
    for (int i = 0; i < m->numDels[action]; i++) {
        int del = m->delLists[action][i];
        os << "      (not (" << su.cleanStr(m->factStrs[del]) << "))" << endl;
    }
    for (pair<int, int> *extra : *cfSet) {
        os << "      (when (and (dfa s" << extra->first << "))";
        os << " (and (dfa s" << extra->second << ")";
        os << " (not (dfa s" << extra->first << "))))" << endl;
    }
    os << "    )" << endl;
    os << "   )" << endl << endl;
}

void ModelWriter::writeEpsilonActionCF(ostream &os, set<pair<int, int> *> *cfSet) {
    os << "   (:action epsilon-" << epsilonAcs++ << endl;
    os << "    :precondition (and )" << endl;
    os << "    :effect (and" << endl;
    for (pair<int, int> *extra : *cfSet) {
        os << "      (when (and (dfa s" << extra->first << "))";
        os << " (and (dfa s" << extra->second << ")";
        os << " (not (dfa s" << extra->first << "))))" << endl;
    }
    os << "    )" << endl;
    os << "   )" << endl << endl;
}

//
// Created by dh on 03.10.20.
//

#include "ModelWriter.h"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;

void ModelWriter::write(Model *htn, FiniteAutomaton *automaton, bool writePDDL, string dName, string pName) {
    this->m = htn;
    this->dfa = automaton;

    if (writePDDL) {
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
    } else { // SAS+
        ofstream os;
        os.open(pName);
        writeSASPlus(os, dfa->getActionMap());
        os.close();
    }
}

void ModelWriter::writeSASPlus(ostream &os, unordered_map<int, unordered_map<int, set<int> *> *> *extraStuff) {
    IntUtil iu;
    iu.sort(m->gList, 0, m->gSize - 1);
    iu.sort(m->s0List, 0, m->s0Size - 1);

    bool isBoolean[m->numVars];
    for (int i = 0; i < m->numVars; i++) {
        isBoolean[i] = (m->lastIndex[i] == m->firstIndex[i]);
    }

    os << "begin_version\n";
    os << "3\n";
    os << "end_version\n";
    os << "begin_metric\n";
    os << "0\n";
    os << "end_metric\n";

    os << m->numVars + 1 << "\n"; // number of variables

    // write original variables
    for (int i = 0; i < m->numVars; i++) {
        os << "begin_variable\n";
        os << "var" << i << "\n";
        os << "-1\n"; // axiom layer
        if (isBoolean[i]) {
            os << "2\n";
            os << "Atom " << sasCleanStr(m->factStrs[m->firstIndex[i]]) << "\n";
            os << "<none of those>\n";
        } else {
            os << (m->lastIndex[i] - m->firstIndex[i] + 1) << "\n";
            for (int j = m->firstIndex[i]; j <= m->lastIndex[i]; j++) {
                string atom = sasCleanStr(m->factStrs[j]);
                if (atom != "<none of those>") {
                    os << "Atom ";
                }
                os << atom << "\n";
            }
        }
        os << "end_variable\n";
    }

    // write dfa states
    os << "begin_variable\n";
    os << "var" << m->numVars << "\n";
    os << "-1\n";
    os << dfa->stateID << "\n";
    for (int i = 0; i < dfa->stateID; i++) {
        os << "Atom dfa(s" << i << ")\n";
    }
    os << "end_variable\n";

    // mutex groups
    os << "0\n";

    // initial state
    os << "begin_state\n";
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
            cout << "error: two values of same sas+ variable are set in s0\n";
            exit(-1);
        }
        s0[var] = val - m->firstIndex[var];
    }
    for (int i = 0; i < m->numVars; i++) {
        if (s0[i] == -1) {
            if (isBoolean[i]) { // it is not set -> set to <none of those>
                s0[i] = 1;
            } else {
                cout << "error: non-boolean variable not set in s0\n";
                exit(-1);
            }
        }
    }
    for (int i = 0; i < m->numVars; i++) {
        os << s0[i] << "\n";
    }
    os << "0\n"; // initial value of automaton
    os << "end_state\n";

    // write goal definition
    os << "begin_goal\n";
    os << m->gSize + 1 << "\n";
    for (int i = 0; i < m->gSize; i++) {
        int val = m->gList[i];
        int var = 0;
        while (!((m->firstIndex[var] <= val) && (m->lastIndex[var] >= val)))
            var++;
        os << var << " " << (val - m->firstIndex[var]) << "\n";
    }
    os << m->numVars << " " << "1\n"; // reach final state of automaton
    os << "end_goal\n";

    // count actions
    int numActions = 0;
    for (int i = -1; i < m->numActions; i++) {
        if (extraStuff->find(i) == extraStuff->end()) {
            continue; // unreachable via automaton
        }
        for(auto from : *extraStuff->at(i)) {
            numActions += from.second->size();
        }
    }
    os << numActions << "\n";

    // write actions
    int* varPrec = new int[m->numVars];
    int* varAdd = new int[m->numVars];
    int* varDel = new int[m->numVars];
    for (int i = 0; i < m->numVars; i++) {
        varPrec[i] = -1;
        varAdd[i] = -1;
        varDel[i] = -1;
    }
    int check = 0;
    vector<int> prevail;
    vector<int> effect;
    for (int i = 0; i < m->numActions; i++) {
        //cout << m->taskNames[i] << "\n";

        if (extraStuff->find(i) == extraStuff->end()) {
            continue; // unreachable via automaton
        }
        // generate FD's SAS+ format
        if (getSASVal(varPrec, m->precLists[i], m->numPrecs[i])) {
            cout << "error: two values of same sas+ variable are in precondition of action " << m->taskNames[i] << endl;
            exit(-1);
        }
        if (getSASVal(varAdd, m->addLists[i], m->numAdds[i])) {
            cout << "error: two values of same sas+ variable are in effect of action " << m->taskNames[i] << endl;
            exit(-1);
        }
        getSASVal(varDel, m->delLists[i], m->numDels[i]);
        /*
        for(int v = 0; v < m->numVars; v++) {
            if ((varPrec[v] != -1)||(varDel[v] != -1)||(varDel[v] != -1)) {
                cout << "- v" << v << " (" << varPrec[v] << " ";
                if (varPrec[v] != -1)
                    cout << m->factStrs[m->firstIndex[v] + varPrec[v]];
                cout << ", " << varAdd[v] << " ";
                if (varAdd[v] != -1)
                    cout << m->factStrs[m->firstIndex[v] + varAdd[v]];
                cout << ", " << varDel[v] << " ";
                if (varDel[v] != -1)
                    cout << m->factStrs[m->firstIndex[v] + varDel[v]];
                cout << ")\n";
            }
        }
         */
        prevail.clear();
        effect.clear();
        for (int v = 0; v < m->numVars; v++) {
            if (isBoolean[v]) {
                if (varPrec[v] != -1) {
                    if (varAdd[v] != -1) { // prevail constraint
                        assert(varPrec[v] == varAdd[v]);
                        prevail.push_back(v);
                        prevail.push_back(varPrec[v]);
                    } else if (varDel[v] != -1) {
                        // this value is deleted -> need to set it to <none of those>
                        effect.push_back(0); // not conditional
                        effect.push_back(v);
                        assert(varPrec[v] == 0);
                        effect.push_back(0); // value needed before
                        effect.push_back(1); // value the variable is set to
                    } else { // prevail constraint
                        assert (varAdd[v] == -1);
                        assert (varDel[v] == -1);
                        prevail.push_back(v);
                        prevail.push_back(varPrec[v]);
                    }
                } else { // prec not set
                    if (varAdd[v] != -1) { // added
                        effect.push_back(0); // not conditional
                        effect.push_back(v);
                        effect.push_back(-1); // value needed before -> do not care
                        assert(varAdd[v] == 0);
                        effect.push_back(0); // value the variable is set to
                    } else if (varDel[v] != -1) {
                        // this value is deleted -> need to set it to <none of those>
                        effect.push_back(0); // not conditional
                        effect.push_back(v);
                        effect.push_back(-1); // value needed before -> do not care
                        effect.push_back(1); // value the variable is set to
                    }
                }
            } else { // is sas+ variable
                if ((varPrec[v] != -1) && (varAdd[v] == -1)) {
                    // prevail constraint
                    prevail.push_back(v);
                    prevail.push_back(varPrec[v]);
                } else if ((varPrec[v] != -1) && (varAdd[v] != -1)) {
                    if (varPrec[v] == varAdd[v]) { // this is actually a prevail constraint
                        prevail.push_back(v);
                        prevail.push_back(varPrec[v]);
                    } else {
                        effect.push_back(0); // not conditional
                        effect.push_back(v);
                        effect.push_back(varPrec[v]); // value needed before
                        effect.push_back(varAdd[v]); // value the variable is set to
                    }
                } else if ((varPrec[v] == -1) && (varAdd[v] != -1)) {
                    effect.push_back(0); // not conditional
                    effect.push_back(v);
                    effect.push_back(-1); // value needed before
                    effect.push_back(varAdd[v]); // value the variable is set to
                } else if ((varPrec[v] != -1) || (varDel[v] != -1) || (varDel[v] != -1)) {
                    cout << "unexpected sas+ values in action:\n";
                    cout << "prec " << varPrec[v] << "\n";
                    cout << "add  " << varAdd[v] << "\n";
                    cout << "del  " << varDel[v] << "\n";
                    exit(-1);
                }
            }
            varPrec[v] = -1;
            varAdd[v] = -1;
            varDel[v] = -1;
        }
        // write everything
        for (auto extra : *extraStuff->at(i)) {
            int from = extra.first;
            for (int to : *extra.second) {
                check++; // count actions actually written
                int numPrevail = prevail.size() / 2;
                int numEffect = (effect.size() / 4);
                if (from != to) {
                    numEffect++;
                } else {
                    numPrevail++;
                }
                os << "begin_operator\n";
                os << m->taskNames[i] << "\n";
                os << numPrevail << "\n";
                for (int j = 0; j < prevail.size(); j += 2) {
                    os << prevail[j] << " " << prevail[j + 1] << "\n";
                }
                if (from == to) { // dfa prec
                    os << m->numVars << " " << from << "\n";
                }
                os << numEffect << "\n"; // one is added for the dfa
                for (int j = 0; j < effect.size(); j += 4) {
                    os << effect[j] << " " << effect[j + 1] << " " << effect[j + 2] << " " << effect[j + 3] << "\n";
                }
                if (from != to) { // dfa effect
                    os << 0 << " " << m->numVars << " " << from << " " << to << "\n";
                }
                os << 1 << "\n"; // action costs, 0 means unicosts
                os << "end_operator\n";
            }
        }
    }
    if (extraStuff->find(-1) != extraStuff->end()) {
        for (auto extra : *extraStuff->at(-1)) {
            int from = extra.first;
            for (int to : *extra.second) {
                assert (from != to);
                check++; // count actions actually written
                os << "begin_operator\n";
                os << "epsilon\n";
                os << 0 << "\n"; // prevail constraints
                os << 1 << "\n"; // effects
                os << 0 << " " << m->numVars << " " << from << " " << to << "\n";
                os << 1 << "\n"; // action costs, 0 means unicosts
                os << "end_operator\n";
            }
        }
    }
    os << 0 << "\n";
    assert(check == numActions);
}

void ModelWriter::writeDomain(ostream &os) {
    os << "(define (domain htn)" << endl;

    cout << "  - writing predicate definition" << endl;
    writePredDef(os, dfa->stateID);

    cout << "  - writing action definition" << endl;
    auto *extraStuff = dfa->getActionMap();
    int tenpercent = m->numActions / 10;
    int since = 0;
    for (int i = 0; i < m->numActions; i++) {
        if (extraStuff->find(i) == extraStuff->end()) {
            cout << "- automaton contains no rule for action " << m->taskNames[i] << endl;
            continue;
        }
        for (auto extra : *extraStuff->at(i)) {
            int from = extra.first;
            for (int to : *extra.second) {
                writeAction(os, i, from, to, from);
            }
        }
        if(++since > tenpercent) {
            cout << "    " << i << "/" << m->numActions << endl;
            since = 0;
        }
        //writeActionCF(os, i, extraStuff[i]);
    }
    //writeEpsilonActionCF(os, extraStuff[-1]);
    if (extraStuff->find(-1) != extraStuff->end()) {
        for (auto extra : *extraStuff->at(-1)) {
            int from = extra.first;
            for (int to : *extra.second) {
                writeEpsilonAction(os, from, to, from);
            }
        }
    } else {
        cout << "- automaton contains no epsilon transitions." << endl;
    }
    os << ")" << endl;
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
        if (m->factStrs[i] == "none-of-them") {
            os << "(none" << i << ")" << endl;
        } else {
            os << "(" << su.cleanStr(m->factStrs[i]) << ")" << endl;
        }
        os << "               ";
    }
    os << "(dfa ?s - dfastate)" << endl;
    os << "  )" << endl << endl;
}

void ModelWriter::writeAction(ostream &os, int action, int dfaPrec, int dfaAdd, int dfaDel) {
    os << "   (:action " << su.cleanStr(m->taskNames[action]) << "-id" << action << endl;
    os << "    :precondition (and" << endl;
    for (int i = 0; i < m->numPrecs[action]; i++) {
        int prec = m->precLists[action][i];
        if (m->factStrs[prec] == "none-of-them" ) {
            os << "      (none" << prec << ")" << endl;
        } else {
            os << "      (" << su.cleanStr(m->factStrs[prec]) << ")" << endl;
        }
    }
    os << "      (dfa s" << dfaPrec << ")";
    os << ")" << endl;
    os << "    :effect (and" << endl;
    for (int i = 0; i < m->numAdds[action]; i++) {
        int add = m->addLists[action][i];
        if (m->factStrs[add] == "none-of-them" ) {
            os << "      (none" << add << ")" << endl;
        } else {
            os << "      (" << su.cleanStr(m->factStrs[add]) << ")" << endl;
        }
    }
    os << "      (dfa s" << dfaAdd << ")" << endl;
    for (int i = 0; i < m->numDels[action]; i++) {
        int del = m->delLists[action][i];
        if (m->factStrs[del] == "none-of-them" ) {
            os << "      (none" << del << ")" << endl;
        } else {
            os << "      (not (" << su.cleanStr(m->factStrs[del]) << "))" << endl;
        }
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
        if (m->factStrs[m->s0List[i]] == "none-of-them" ) {
            os << "      (none" << m->s0List[i] << ")" << endl;
        } else {
            os << "      (" << su.cleanStr(m->factStrs[m->s0List[i]]) << ")" << endl;
        }
    }
    os << "      (dfa s" << dfa->startState << ")" << endl;
    os << "   )" << endl;
    os << "   (:goal (and" << endl;
    for (int i = 0; i < m->gSize; i++) {
        if (m->factStrs[m->gList[i]] == "none-of-them" ) {
            os << "      (none" << m->gList[i] << ")" << endl;
        } else {
            os << "      (" << su.cleanStr(m->factStrs[m->gList[i]]) << ")" << endl;
        }
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

string ModelWriter::sasCleanStr(string s) {
    if(s == "none-of-them") {
        return "<none of those>";
    }
    return s;
}

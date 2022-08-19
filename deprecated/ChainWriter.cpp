//
// Created by dh on 26.11.20.
//

#include <fstream>
#include <cassert>
#include <map>
#include "ChainWriter.h"

void ChainWriter::write(Model *htn, FiniteAutomaton *automaton, bool writePDDL, string dFile, string pFile) {
    cout << "- start writing" << endl;
    this->m = htn;
    this->dfa = automaton;
    ofstream os;
    os.open(pFile);
    writeSASPlus(os);
    os.close();
}

void ChainWriter::writeSASPlus(ostream &os) {
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

    cout << "- generation FD's SAS+ representation" << endl;
    // write actions
    int *varPrec = new int[m->numVars];
    int *varAdd = new int[m->numVars];
    int *varDel = new int[m->numVars];
    for (int i = 0; i < m->numVars; i++) {
        varPrec[i] = -1;
        varAdd[i] = -1;
        varDel[i] = -1;
    }
    vector<int> *prevail = new vector<int>[m->numActions];
    vector<int> *effect = new vector<int>[m->numActions];
    for (int i = 0; i < m->numActions; i++) {
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

        for (int v = 0; v < m->numVars; v++) {
            if (isBoolean[v]) {
                if (varPrec[v] != -1) {
                    if (varAdd[v] != -1) { // prevail constraint
                        assert(varPrec[v] == varAdd[v]);
                        prevail[i].push_back(v);
                        prevail[i].push_back(varPrec[v]);
                    } else if (varDel[v] != -1) {
                        // this value is deleted -> need to set it to <none of those>
                        effect[i].push_back(v);
                        assert(varPrec[v] == 0);
                        effect[i].push_back(0); // value needed before
                        effect[i].push_back(1); // value the variable is set to
                    } else { // prevail constraint
                        assert (varAdd[v] == -1);
                        assert (varDel[v] == -1);
                        prevail[i].push_back(v);
                        prevail[i].push_back(varPrec[v]);
                    }
                } else { // prec not set
                    if (varAdd[v] != -1) { // added
                        effect[i].push_back(v);
                        effect[i].push_back(-1); // value needed before -> do not care
                        assert(varAdd[v] == 0);
                        effect[i].push_back(0); // value the variable is set to
                    } else if (varDel[v] != -1) {
                        // this value is deleted -> need to set it to <none of those>
                        effect[i].push_back(v);
                        effect[i].push_back(-1); // value needed before -> do not care
                        effect[i].push_back(1); // value the variable is set to
                    }
                }
            } else { // is sas+ variable
                if ((varPrec[v] != -1) && (varAdd[v] == -1)) {
                    // prevail constraint
                    prevail[i].push_back(v);
                    prevail[i].push_back(varPrec[v]);
                } else if ((varPrec[v] != -1) && (varAdd[v] != -1)) {
                    if (varPrec[v] == varAdd[v]) { // this is actually a prevail constraint
                        prevail[i].push_back(v);
                        prevail[i].push_back(varPrec[v]);
                    } else {
                        effect[i].push_back(v);
                        effect[i].push_back(varPrec[v]); // value needed before
                        effect[i].push_back(varAdd[v]); // value the variable is set to
                    }
                } else if ((varPrec[v] == -1) && (varAdd[v] != -1)) {
                    effect[i].push_back(v);
                    effect[i].push_back(-1); // value needed before
                    effect[i].push_back(varAdd[v]); // value the variable is set to
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
    }
    delete[] varPrec;
    delete[] varAdd;
    delete[] varDel;


    cout << "- building macro action candidates" << endl;
    vector<macroAction *> *macroActions = new vector<macroAction *>;
    set<int> reachedStates;
    reachedStates.insert(0);
    vector<int> *stack = new vector<int>;
    stack->push_back(0);
    while (!stack->empty()) {
        int sFrom = stack->back();
        stack->pop_back();
        if (dfa->fda2.find(sFrom) == dfa->fda2.end())
            continue;
        unordered_map<int, set<int> *> *toSet = dfa->fda2.at(sFrom);
        macroAction *ma;
        for (auto to : *toSet) { // for each target state
            int sTo = to.first;
            for (int a : *to.second) { // for each action leading there
                ma = new macroAction(sFrom);
                if(a != -1) { // epsilon transition
                    ma->add(a);
                }
                set<int> visitedInThisRun;
                visitedInThisRun.insert(sTo);
                if(dfa->fda2.find(sTo) != dfa->fda2.end()) {
                    unordered_map<int, set<int> *> *nextToStates = dfa->fda2.at(sTo);
                    // single target start + single action for this transition
                    while (((nextToStates->size() == 1) && (nextToStates->begin()->second->size() == 1))) {
                        int a2 = *nextToStates->begin()->second->begin();
                        if (a2 != -1) { // epsilon transition
                            ma->add(a2);
                        }
                        sTo = nextToStates->begin()->first;
                        if (isFinalState(sTo)) {
                            break;
                        }
                        if (dfa->fda2.find(sTo) == dfa->fda2.end()) {
                            break;
                        }
                        if(visitedInThisRun.find(sTo) != visitedInThisRun.end()) {
                            break;
                        }
                        visitedInThisRun.insert(sTo);
                        nextToStates = dfa->fda2.at(sTo);
                    }
                }
                ma->sTo = sTo;
                if(ma->aSeq->size() == 0) { // only epsilon transitions
                    ma->aSeq->push_back(-1); // need to insert one
                }
                macroActions->push_back(ma);

                if (reachedStates.find(sTo) == reachedStates.end()) {
                    stack->push_back(sTo);
                    reachedStates.insert(sTo);
                }
            }
        }
    }
    delete stack;

    //printGraph(macroActions);

    cout << "- building macro actions" << endl;
    vector<int> deletedMAs;
    for (int i = 0; i < macroActions->size(); i++) {
        macroAction *ma = macroActions->at(i);

        // initializing preconditions and effects with those of first action
        int a0 = ma->aSeq->at(0);
        ma->precs = new map<int, int>;
        ma->effects = new map<int, int>;

        if(a0 != -1) {
            for (int j = 0; j < prevail[a0].size(); j += 2) {
                setTo(ma->precs, prevail[a0][j], prevail[a0][j + 1]);
            }
            for (int j = 0; j < effect[a0].size(); j += 3) {
                if (effect[a0][j + 1] != -1) { // this is the "I don't care value"
                    setTo(ma->precs, effect[a0][j], effect[a0][j + 1]);
                }
                setTo(ma->effects, effect[a0][j], effect[a0][j + 2]);
            }
        }

        // increment state
        for (int j = 1; j < ma->aSeq->size(); j++) {
            int a = ma->aSeq->at(j);
            map<int, int> *newPrecs = new map<int, int>;
            map<int, int> *newEffs = new map<int, int>;
            for (int k = 0; k < prevail[a].size(); k += 2) {
                setTo(newPrecs, prevail[a][k], prevail[a][k + 1]);
            }
            for (int k = 0; k < effect[a].size(); k += 3) {
                if(effect[a][k + 1] != -1) { // this is the "I don't care value"
                    setTo(newPrecs, effect[a][k], effect[a][k + 1]);
                }
                setTo(newEffs, effect[a][k], effect[a][k + 2]);
            }

            for (auto prec : *newPrecs) {
                int var = prec.first;
                int val = prec.second;
                if (ma->effects->find(var) != ma->effects->end()) {
                    if (ma->effects->at(var) != val) { // an action before puts it to the wrong value
                        ma->isBroken = true;
                        break;
                    } // else: an action before makes it true -> nothing to do
                } else { // this variable is not touched by any action before
                    if (ma->precs->find(var) != ma->precs->end()) {
                        if (ma->precs->at(var) != val) { // action before needs other value
                            ma->isBroken = true;
                            break;
                        } // else: other action before has same prec -> nothing to do
                    } else { // need to add it as new prec
                        setTo(ma->precs,var, val);
                    }
                }
            }
            if (ma->isBroken) {
                deletedMAs.push_back(i);
                break;
            }
            for (auto eff : *newEffs) {
                int var = eff.first;
                int val = eff.second;
                setTo(ma->effects, var, val);
            }
        }
    }
    delete[] prevail;
    delete[] effect;

    int numActions = macroActions->size() - deletedMAs.size();
    cout << "- reduced action set from " << dfa->numTransitions << " actions to " << numActions << " macro actions"<< endl;
    cout << "- reduced state set from " << dfa->stateID << " to " << reachedStates.size() << " dfa states" << endl;
    cout << "- discarded " << deletedMAs.size() << " broken macro actions" << endl;

    // write dfa states
    os << "begin_variable\n";
    os << "var" << m->numVars << "\n";
    os << "-1\n";
    os << reachedStates.size() << "\n";
    map<int, int> stateMapping;
    stateMapping[0] = 0;
    os << "Atom dfa(s" << 0 << ")\n";
    stateMapping[1] = 1;
    os << "Atom dfa(s" << 1 << ")\n";
    reachedStates.erase(0);
    reachedStates.erase(1);
    int j = 2;
    for (int i : reachedStates) {
        os << "Atom dfa(s" << i << ")\n";
        stateMapping[i] = j++;
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

    cout << "- writing macro actions" << endl;
    int check = 0;
    os << numActions << "\n";
    set<int> prev;
    set<int> eff;
    for(auto ma : *macroActions) {
        if(ma->isBroken) {
            //delete ma;
            continue;
        }
        int from = stateMapping[ma->sfrom];
        int to = stateMapping[ma->sTo];
        prev.clear();
        eff.clear();

        for(auto p : *ma->precs) {
            int var = p.first;
            if (ma->effects->find(var) == ma->effects->end()) {
                prev.insert(var);
            } else {
                eff.insert(var);
            }
        }
        for(auto p : *ma->effects) {
            int var = p.first;
            eff.insert(var);
        }

        int numPrevail = prev.size();
        int numEffect = eff.size();
        if (from != to) {
            numEffect++;
        } else {
            numPrevail++;
        }
        os << "begin_operator " << "\n";
        for(int j = 0; j < ma->aSeq->size(); j++) {
            int a = ma->aSeq->at(j);
            if(j > 0) {
                os << ")(";
            }
            if(a >= 0) {
                os << m->taskNames[a];
            } else {
                os << "epsilon";
            }
        }
        os << "\n";

        os << numPrevail << "\n";
        for (int var : prev) {
            os << var << " " << ma->precs->at(var) << "\n";
        }
        if (from == to) { // dfa prec
            os << m->numVars << " " << from << "\n";
        }
        os << numEffect << "\n"; // one is added for the dfa
        for (int var : eff) {
            os << "0 " << var << " "; // no condition; the var
            if(ma->precs->find(var) == ma->precs->end()){
                os << "-1";
            } else {
                os << ma->precs->at(var);
            }
            os << " " << ma->effects->at(var) << "\n";
        }
        if (from != to) { // dfa effect
            os << 0 << " " << m->numVars << " " << from << " " << to << "\n";
        }
        os << 1 << "\n"; // action costs, 0 means unicosts
        os << "end_operator\n";
        check++;
        delete ma;
    }
    os << "0\n";
    if(check != numActions) {
        cout << "TOAD-ERROR: FAILED TO WRITE MODEL" << endl;
        cout << "CHECK: " << check;
        os.flush();
        exit(-1);
    }
    delete macroActions;
}

void ChainWriter::printGraph(vector<macroAction *> *macroActions) const {
    cout << "digraph G {" << endl;
    for(int i = 0; i < macroActions->size(); i++) {
        macroAction* ma = macroActions->at(i);

        cout << "n" << ma->sfrom << " -> n" << ma->sTo << "[label=\"";
        for(int j = 0; j < ma->aSeq->size(); j++) {
            cout << ma->aSeq->at(j);
            if(j < ma->aSeq->size() - 1) {
                cout << ", ";
            }
        }
        cout << "\"];" << endl;
    }
    cout << "}" << endl;
}

string ChainWriter::sasCleanStr(string s) {
    return s;
}

bool ChainWriter::getSASVal(int *store, int *somelist, int length) {
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

bool ChainWriter::isFinalState(int s) {
    return s == 1;
}

void ChainWriter::setTo(map<int, int> *someMap, int &key, int &val) {
    if(someMap->find(key) == someMap->end()){
        someMap->insert({key, val});
    } else{
        someMap->at(key) = val;
    }
}



/*
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
assert(check == numActions);*/
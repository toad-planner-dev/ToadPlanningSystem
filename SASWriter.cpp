//
// Created by dh on 07.04.21.
//

#include "SASWriter.h"
#include <cassert>
#include <fstream>
#include <sys/time.h>

void SASWriter::write(Model *htn, StdVectorFst *fst, string dName, string pName) {
    this->m = htn;

    ofstream os;
    os.open(pName);
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

    // check number of final states
    vector<int> dfaGoalStates;
    for (int i = 0; i < fst->NumStates(); i++) {
        if (fst->Final(i) == 0) {
            dfaGoalStates.push_back(i);
        }
    }
    int numDFAStates = fst->NumStates();
    int singleDFAGoalState = dfaGoalStates[0];;
    int additionalActions = 0;
    if (dfaGoalStates.size() > 1) {
        singleDFAGoalState = numDFAStates; // the new one
        numDFAStates++;
        additionalActions = dfaGoalStates.size(); // need new epsilon transitions
        addedState = true; // this value is used when the heuristic lookup table is computed
    }

    // write dfa states
    os << "begin_variable\n";
    os << "var" << m->numVars << "\n";
    os << "-1\n";
    if (numDFAStates > 1) {
        os << numDFAStates << "\n";
        for (int i = 0; i < numDFAStates; i++) {
            os << "Atom dfa(s" << i << ")\n";
        }
    } else { // FD cannot deal with FDR groups of size 1
        assert(numDFAStates == 1);
        os << "2\n";
        os << "Atom dfa(s0)\n";
        os << "<none of those>\n";
        addedState = true;
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
    os << fst->Start() << "\n"; // initial value of automaton
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
    os << m->numVars << " " << singleDFAGoalState << "\n"; // reach final state of automaton
    os << "end_goal\n";

    // count actions
    cout << "- counting actions..." << endl;
    timeval tp;
    gettimeofday(&tp, NULL);
    long beginCount = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    int numActions = 0;
    for (StateIterator<StdVectorFst> siter(*fst); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        for (ArcIterator<StdFst> aiter(*fst, state_id); !aiter.Done(); aiter.Next()) {
            numActions++;
        }
    }
    gettimeofday(&tp, NULL);
    long endCount = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timeCountingActions=" << (endCount - beginCount) << "]" << endl;
    cout << "- [numActions=" << numActions << "]" << endl;

    numActions += additionalActions;
    os << numActions << "\n";

    // generate SAS+ representation
    int ** prev = new int*[m->numActions];
    int * numPrev = new int[m->numActions];
    int ** eff = new int*[m->numActions];
    int * numEff = new int[m->numActions];

    getSASRepresentation(isBoolean, prev, numPrev, eff, numEff);

    // write actions
//    fa->delta->fullIterInit();
//    tStateID from, to;
//    tLabelID a;
//    while (fa->delta->fullIterNext(&from, &a, &to)) {
//        assert((a < m->numActions) || (a == epsilon));
//    unordered_set<string> actions;
    for (StateIterator<StdVectorFst> siter(*fst); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        for (ArcIterator<StdFst> aiter(*fst, state_id); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int a = arc.ilabel - 1;
            assert(a < htn->numActions);
            int from = state_id;
            int to = arc.nextstate;

//            string strName = to_string(from) + "#" + to_string(to) + "#" + to_string(a);
//            if (actions.find(strName) != actions.end()) {
//                cout << "found duplicate" << endl;
//                exit(0);
//            }
//            actions.insert(strName);

            if (a == -1) { // epsilon
                assert (from != to);
                os << "begin_operator\n";
                os << "epsilon\n";
                os << 0 << "\n"; // prevail constraints
                os << 1 << "\n"; // effects
                os << 0 << " " << m->numVars << " " << from << " " << to << "\n";
                os << 1 << "\n"; // action costs, 0 means unicosts
                os << "end_operator\n";
                continue;
            }

            // write everything
            int numPrevail = numPrev[a] / 2;
            int numEffect = (numEff[a] / 4);
            if (from != to) {
                numEffect++;
            } else {
                numPrevail++;
            }
            os << "begin_operator\n";
            os << m->taskNames[a] << "\n";
            os << numPrevail << "\n";
            for (int j = 0; j < numPrev[a]; j += 2) {
                os << prev[a][j] << " " << prev[a][j + 1] << "\n";
            }
            if (from == to) { // dfa prec
                os << m->numVars << " " << from << "\n";
            }
            os << numEffect << "\n"; // one is added for the dfa
            for (int j = 0; j < numEff[a]; j += 4) {
                os << eff[a][j] << " " << eff[a][j + 1] << " " << eff[a][j + 2] << " " << eff[a][j + 3] << "\n";
            }
            if (from != to) { // dfa effect
                os << 0 << " " << m->numVars << " " << from << " " << to << "\n";
            }
            os << 1 << "\n"; // action costs, 0 means unicosts
            os << "end_operator\n";
        }
    }
    if (dfaGoalStates.size() > 1) {
        for (int orgDFAGoal: dfaGoalStates) {
            os << "begin_operator\n";
            os << "epsilon\n";
            os << 0 << "\n"; // prevail constraints
            os << 1 << "\n"; // effects
            os << 0 << " " << m->numVars << " " << orgDFAGoal << " " << singleDFAGoalState << "\n";
            os << 1 << "\n"; // action costs, 0 means unicosts
            os << "end_operator\n";
        }
    }
    os << 0 << "\n";
    os.close();
}

void SASWriter::getSASRepresentation(const bool *isBoolean, int **prev, int *numPrev, int **eff, int *numEff) {
    int* varPrec = new int[m->numVars];
    int* varAdd = new int[m->numVars];
    int* varDel = new int[m->numVars];
    for (int i = 0; i < m->numVars; i++) {
        varPrec[i] = -1;
        varAdd[i] = -1;
        varDel[i] = -1;
    }

    vector<int> prevail;
    vector<int> effect;
    for(int a = 0; a < m->numActions; a++) {
        // generate FD's SAS+ format
        if (getSASVal(varPrec, m->precLists[a], m->numPrecs[a])) {
            cout << "error: two values of same sas+ variable are in precondition of action " << m->taskNames[a]
                 << endl;
            exit(-1);
        }
        if (getSASVal(varAdd, m->addLists[a], m->numAdds[a])) {
            cout << "error: two values of same sas+ variable are in effect of action " << m->taskNames[a]
                 << endl;
            exit(-1);
        }
        getSASVal(varDel, m->delLists[a], m->numDels[a]);
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
        numPrev[a] = prevail.size();
        prev[a] = new int[prevail.size()];
        for(int i = 0; i < prevail.size(); i++) {
            prev[a][i] = prevail[i];
        }
        numEff[a] = effect.size();
        eff[a] = new int[effect.size()];
        for(int i = 0; i < effect.size(); i++) {
            eff[a][i] = effect[i];
        }
    }
}

string SASWriter::sasCleanStr(string s) {
    if(s == "none-of-them") {
        return "<none of those>";
    }
    return s;
}

bool SASWriter::getSASVal(int *store, int *somelist, int length) {
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

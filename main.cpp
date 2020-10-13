#include <iostream>
#include "Model.h"
#include "translation/FiniteAutomaton.h"
#include "translation/CFGtoFDAtranslator.h"
#include "ModelWriter.h"
#include <vector>
#include <cassert>

vector<int> *mToRule(const Model *htn, int iM);

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
    std::cout << "Translating Total Order HTN model to STRIPS model." << std::endl;

#ifndef NDEBUG
    cout
            << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug."
            << endl;
#endif
    string s;
    int seed = 42;
    if (argc == 1) {
        cout << "No file name passed. Reading input from stdin";
        s = "stdin";
    } else {
        s = argv[1];
        if (argc > 2) seed = atoi(argv[2]);
    }
    cout << "Random seed: " << seed << endl;
    srand(seed);


/*
 * Read model
 */
    cout << "Reading HTN model from file \"" << s << "\" ... " << endl;
    Model *htn = new Model();
    htn->filename = s;
    htn->read(s);
    //assert(htn->isHtnModel);

    htn->calcSCCs();
    //htn->calcSCCGraph();
/*
 * Building grammar
 */
    cout << "Starting Translation" << endl;
    CFGtoFDAtranslator *to2s = new CFGtoFDAtranslator();
    to2s->numSymbols = htn->numTasks;
    to2s->numTerminals = htn->numActions;

    // initialize Ni sets
    to2s->NumNis = htn->numCyclicSccs;
    to2s->NiSize = new int[htn->numCyclicSccs];
    to2s->Ni = new int*[htn->numCyclicSccs];
    to2s->SymToNi = new int[to2s->numSymbols];
    for(int i = 0; i < to2s->numSymbols; i++) {
        to2s->SymToNi[i] = -1; // init as non-recursive
    }

    cout << "- collecting SCC data" << endl;
    for (int k = 0; k < htn->numCyclicSccs; k++) {
        int scc = htn->sccsCyclic[k];
        to2s->NiSize[k] = htn->sccSize[scc];
        to2s->Ni[k] = new int[to2s->NiSize[k]];
        for (int j = 0; j < htn->sccSize[scc]; j++) {
            to2s->Ni[k][j] = htn->sccToTasks[scc][j];
            to2s->SymToNi[to2s->Ni[k][j]] = k;
        }
    }

    cout << "- adding methods as grammar rules" << endl;
    for (int iM = 0; iM < htn->numMethods; iM++) {
        vector<int> *rule = mToRule(htn, iM);
        to2s->addRule(rule);
    }
    cout << "- analysing rules" << endl;
    to2s->analyseRules();
    int S = htn->initialTask;

    cout << endl << "Creating DFA..." << endl;
    cout << "- Starting symbol: " << S << endl;

    to2s->dfa->startState = to2s->dfa->stateID++;
    to2s->dfa->finalState = to2s->dfa->stateID++;
    to2s->makeFA(to2s->dfa->startState, S, to2s->dfa->finalState);
    cout << "- done!" << endl;

    cout << "creating output STRIPS model" << endl;
    string dFile = "/home/dh/Schreibtisch/temp3/sas/domain.pddl";
    string pFile ="/home/dh/Schreibtisch/temp3/sas/problem.pddl";

    ModelWriter mw;
    mw.write(htn, to2s->dfa, dFile, pFile);
    cout << "done!" << endl;
    //mw.dfa->print(htn->taskNames, 0, 1);

    //to2s->dfa.print(htn->taskNames, startState, finalState);
    return 0;
}

vector<int> *mToRule(const Model *htn, int iM) {
    bool printDebug = false;
    if(printDebug) {
        cout << "----" << endl;
        cout << "d: " << htn->decomposedTask[iM] << " " << htn->taskNames[htn->decomposedTask[iM]] << endl;
        for (int i = 0; i < htn->numSubTasks[iM]; i++) {
            int st = htn->subTasks[iM][i];
            cout << i << " " << st << " " << htn->taskNames[st] << endl;
        }
        for(int i = 0; i < htn->numOrderings[iM]; i+= 2) {
            cout << htn->ordering[iM][i] << " < " << htn->ordering[iM][i + 1] << endl;
        }
    }

    vector<int> *rule = new vector<int>;
    rule->push_back(htn->decomposedTask[iM]);

    set<int> done;
    set<int> sts;
    for (int iST = 0; iST < htn->numSubTasks[iM]; iST++) {
        sts.insert(iST);
    }

    while (!sts.empty()) {
        int first = -1;
        for (int iST : sts) {
            bool unconstrained = true;
            for (int o = 0; o < htn->numOrderings[iM]; o += 2) {
                int p = htn->ordering[iM][o];
                int s = htn->ordering[iM][o + 1];
                if ((s == iST) && (done.find(p) == done.end())) {
                    unconstrained = false;
                    break;
                }
            }
            if (unconstrained) {
                if (first < 0) {
                    first = iST;
                } else {
                    cout << "ERROR: The method " << htn->methodNames[iM]
                         << " is partially ordered. Only totally ordered problems are supported." << endl;
                    exit(-1);
                }
            }
        }
        assert(first >= 0);
        sts.erase(first);
        done.insert(first);
        int st = htn->subTasks[iM][first];
        rule->push_back(st);
    }

    if(printDebug) {
        cout << rule->at(0) << " -> ";
        for (int i = 1; i < rule->size(); i++)
            cout << rule->at(i) << " ";
        cout << endl;
    }
    return rule;
}

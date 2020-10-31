#include <iostream>
#include "htnModel/Model.h"
#include "translation/FiniteAutomaton.h"
#include "translation/CFGtoFDAtranslator.h"
#include "ModelWriter.h"
#include "optimization/RPGReachability.h"
#include "verification/GroundVerifier.h"
#include <vector>
#include <cassert>
#include <sys/time.h>

vector<int> *mToRule(const Model *htn, int iM);

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
    std::cout << "TOAD - Total Order HTN Approximation with DFA." << std::endl;

#ifndef NDEBUG
    cout
            << "You have compiled TOAD without setting the NDEBUG flag. This will make it slow and should only be done for debug."
            << endl;
#endif
    string s;
    timeval tp;

    bool verify = false;
    int seed = 42;
    bool printhelp = false;
    if (argc < 2) {
        printhelp = true;
    } else {
        string str = argv[1];
        if (str == "-v") {
            verify = true;
            s = argv[2];
            if (argc != 4) printhelp = true;
        } else {
            s = argv[1];
            if (argc == 3) seed = atoi(argv[2]);
        }
    }

    if (printhelp){
        cout << "usage:" << endl;
        cout << "toad pandagrounding [seed]" << endl;
        cout << "toad -v pandagrounding sasplan" << endl;
        exit(-1);
    }

    cout << "Random seed: " << seed << " [rseed=" << seed << "]" << endl;
    srand(seed);

    /*
    * Read model
    */
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    long startTotal = startT;
    cout << "Reading HTN model from file \"" << s << "\" ... " << endl;
    Model *htn = new Model();
    htn->filename = s;
    htn->read(s);
    //assert(htn->isHtnModel);

    htn->calcSCCs();
    gettimeofday(&tp, NULL);
    long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timePrepareModel=" << (endT - startT) << "]" << endl;
    startT = endT;

    if (verify) {
        /*
        * Creating verify problem
        */
        cout << "Creating verify problem." << endl;
        GroundVerifier v;
        string sasPlan = argv[3];
        v.verify(htn, sasPlan);
        gettimeofday(&tp, NULL);
        long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "- [timeCreatingVerifyproblem=" << (endT - startT) << "]" << endl;
        exit(0);
    }

    /*
    * Building grammar
    */
    cout << "Starting translation" << endl;
    CFGtoFDAtranslator *to2s = new CFGtoFDAtranslator();
    to2s->numSymbols = htn->numTasks;
    to2s->numTerminals = htn->numActions;

    // initialize Ni sets
    to2s->NumNis = htn->numCyclicSccs;
    to2s->NiSize = new int[htn->numCyclicSccs];
    to2s->Ni = new int *[htn->numCyclicSccs];
    to2s->SymToNi = new int[to2s->numSymbols];
    for (int i = 0; i < to2s->numSymbols; i++) {
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
    cout << "Analysing rules" << endl;
    to2s->analyseRules();
    int S = htn->initialTask;
    if (!to2s->isRegurlar) {
        return -7;
    }

    cout << "Creating DFA" << endl;
    cout << "- starting symbol: " << S << endl;
    cout << "- building dfa..." << endl;
    to2s->dfa->startState = to2s->dfa->stateID++;
    to2s->dfa->finalState = to2s->dfa->stateID++;
    to2s->makeFA(to2s->dfa->startState, S, to2s->dfa->finalState);
    cout << "- dfa states " << to2s->dfa->stateID << ". [dfaSraw=" << to2s->dfa->stateID << "]" << endl;
    cout << "- dfa transitions " << to2s->dfa->numTransitions << ". [dfaTraw=" << to2s->dfa->numTransitions << "]"
         << endl;
    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timeBuildingDFA=" << (endT - startT) << "]" << endl;
    startT = endT;

    //cout << "Performing delete-relaxed forward reachability analysis" << endl;
    //RPGReachability *rpg = new RPGReachability(htn);
    //rpg->computeReachability(to2s->dfa);

    cout << "Creating output STRIPS model" << endl;
    string dFile = "domain.pddl";
    string pFile = "problem.pddl";

    ModelWriter mw;
    bool writePDDL = false; // PDDL or SAS+
    if (writePDDL)
        cout << "- Writing PDDL representation. [writer=PDDL]" << endl;
    else
        cout << "- Writing FD's SAS+ representation. [writer=SAS]" << endl;
    mw.write(htn, to2s->dfa, writePDDL, dFile, pFile);
    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timeWritingModel=" << (endT - startT) << "]" << endl;
    cout << "- [timeTotal=" << (endT - startTotal) << "]" << endl;

    cout << "Finished!" << endl;
    //mw.dfa->print(htn->taskNames, 0, 1);

    //to2s->dfa.print(htn->taskNames, startState, finalState);
    return 0;
}

vector<int> *mToRule(const Model *htn, int iM) {
    bool printDebug = false;
    if (printDebug) {
        cout << "----" << endl;
        cout << "d: " << htn->decomposedTask[iM] << " " << htn->taskNames[htn->decomposedTask[iM]] << endl;
        for (int i = 0; i < htn->numSubTasks[iM]; i++) {
            int st = htn->subTasks[iM][i];
            cout << i << " " << st << " " << htn->taskNames[st] << endl;
        }
        for (int i = 0; i < htn->numOrderings[iM]; i += 2) {
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

    if (printDebug) {
        cout << rule->at(0) << " -> ";
        for (int i = 1; i < rule->size(); i++)
            cout << rule->at(i) << " ";
        cout << endl;
    }
    return rule;
}

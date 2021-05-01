#include <iostream>
#include "htnModel/Model.h"
#include "translation/FiniteAutomaton.h"
#include "translation/CFGtoFDAtranslator.h"
#include "ModelWriter.h"
#include "optimization/RPGReachability.h"
#include "verification/GroundVerifier.h"
#include "translation/TailRecAnalysis.h"
#include "optimization/DFAMinimization.h"
#include "translation/CFtoRegGrammarEnc.h"
#include "ChainWriter.h"
#include "utils/TripleSet.h"
#include "BinSetWriter.h"
#include "hDfaDistance.h"
#include "dfsDelRelReachability.h"
#include "dfaLib/FA.h"
#include "SASWriter.h"
#include <vector>
#include <cassert>
#include <sys/time.h>

vector<int> *mToRule(const Model *htn, int iM);

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {

    std::cout << "TOAD - Total Order HTN Approximation with DFA." << std::endl;
//
//    FA fa;
//
//    //fa.compileToDFA();
//
//    /*
//    // test minimization
//    fa.sInit.insert(0);
//    fa.sGoal.insert(2);
//    fa.sGoal.insert(3);
//    fa.sGoal.insert(4);
//    fa.numStates = 6;
//    fa.numSymbols = 2;
//    //fa.delta->ensureFW();
//    fa.addRule(0,0,1);
//    fa.addRule(0,1,2);
//    fa.addRule(1,0,0);
//    fa.addRule(1,1,3);
//    fa.addRule(2,0,4);
//    fa.addRule(2,1,5);
//    fa.addRule(3,0,4);
//    fa.addRule(3,1,5);
//    fa.addRule(4,0,4);
//    fa.addRule(4,1,5);
//    fa.addRule(5,0,5);
//    fa.addRule(5,1,5);
//    fa.printRules();
//     */
//
//    fa.sInit.insert(0);
//    fa.sGoal.insert(1);
//    fa.numStates = 4;
//    fa.numSymbols = 2;
//    fa.addRule(0,0,0);
//    fa.addRule(0,0,2);
//    fa.addRule(0,1,0);
//    fa.addRule(2,0,3);
//    fa.addRule(2,1,3);
//    fa.addRule(3,0,1);
//    fa.addRule(3,1,1);
//
//    //fa.delta.
//    //fa.printDOT();
//    fa.compileToDFA();
//    fa.minimize();
//    //fa.printRules();
//    fa.printDOT();
//
//    exit(0);

#ifndef NDEBUG
    cout
            << "You have compiled TOAD without setting the NDEBUG flag. This will make it slow and should only be done for debug."
            << endl;
#endif
    string s;
    timeval tp;

    bool verify = false;
    bool determineIfTR = false;
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
        } else if (str == "-tr") {
            determineIfTR = true;
            s = argv[2];
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
    if (!verify) {
        htn->calcSCCs();
    }
    gettimeofday(&tp, NULL);

//    if (determineIfTR) {
//        TailRecAnalysis tra;
//        string filename = "/home/dh/Dokumente/versioniert/Source-Code/TOAD-Source/examples/testTR.lp";
//        tra.analyse(htn);
//        exit(0);
//    }

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
    to2s->initDataStructures();
    to2s->analyseRules(true);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timeHtnToGrammar=" << (endT - startT) << "]" << endl;
    startT = endT;

    int S = htn->initialTask;
    //to2s->printRules();
    if (!to2s->isRegurlar) {
        //to2s->printRules();
        CFtoRegGrammarEnc approx;
        cout << "- re-encode rules" << endl;
        approx.overapproximate(to2s, htn);
        cout << "- init data structures" << endl;
        to2s->initDataStructures();
        cout << "- calc SCCs" << endl;
        to2s->calcSCCs(htn->initialTask);
        cout << "- re-analysing rules" << endl;
        to2s->analyseRules(true);
        //to2s->printRules();
        endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "- [timeCfgToRegTransf=" << (endT - startT) << "]" << endl;
        startT = endT;
    }
    //to2s->printRules();

    //to2s->printRules();
    FA* fa = to2s->makeFABU(htn, htn->initialTask);
    //fa->showDOT();
    //fa->showDOT(htn->taskNames);
    //exit(0);
    string dFile2 = "domain.pddl";
    string pFile2 = "problem.sas";

    SASWriter mw2;
    mw2.write(htn, fa, dFile2, pFile2);

    exit(0);

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

    /*
    cout << "Minimizing DFA" << endl;
    DFAMinimization mini;
    mini.minimize(htn, to2s->dfa, to2s->dfa->startState, to2s->dfa->finalState);
    cout << "done!" << endl;

    cout << "Performing delete-relaxed forward reachability analysis" << endl;
    RPGReachability *rpg = new RPGReachability(htn);
    rpg->computeReachability(to2s->dfa);
    */

    //string hFile = "dfad.heuristic";
    //hDfaDistance dfad;
    //dfad.write(hFile, to2s->dfa, htn);
    dfsDelRelReachability drReachability;
    drReachability.reachabilityAnalysis(to2s->dfa, htn);

    cout << "Creating output model" << endl;
    string dFile = "domain.pddl";
    string pFile = "problem.sas";

    ModelWriter mw;
    //ChainWriter mw;
    //BinSetWriter mw;
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
    //delete to2s;

    cout << "Finished!" << endl;

    //mw.dfa->print(htn->taskNames, 0, 1);
    //htn->printTDG();
    delete htn;

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

#include <iostream>
#include "htnModel/Model.h"
#include "translation/FiniteAutomaton.h"
#include "translation/CFGtoFDAtranslator.h"
#include "translation/TailRecAnalysis.h"
#include "translation/CFtoRegGrammarEnc.h"
#include "dfaLib/FA.h"
#include "SASWriter.h"
#include "ModelWriter.h"
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

    int seed = 42;
    bool printhelp = false;
    if (argc < 2) {
        printhelp = true;
    } else {
        string str = argv[1];
        s = argv[1];
        if (argc == 3) seed = atoi(argv[2]);
    }

    if (printhelp){
        cout << "usage:" << endl;
        cout << "toad pandagrounding [seed]" << endl;
        exit(-1);
    }

    cout << "Random seed: " << seed << " [rseed=" << seed << "]" << endl;
    srand(seed);

    /*
    * Read model
    */
    gettimeofday(&tp, NULL);
    long startT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Reading HTN model from file \"" << s << "\" ... " << endl;
    Model *htn = new Model();
    htn->filename = s;
    htn->read(s);
//    htn->calcSCCs();
    gettimeofday(&tp, NULL);

    long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timePrepareModel=" << (endT - startT) << "]" << endl;
    startT = endT;



//    // this is not TOAD <BEGIN>
//    SASWriter mw3;
//    string pFile3 = "problem.sas";
//    mw3.write2(htn, 5, pFile3);
//    exit(0);
//    // this is not TOAD <END>


    /*
    * Building grammar
    */
    cout << "Starting translation" << endl;
    CFGtoFDAtranslator *to2s = new CFGtoFDAtranslator();
    to2s->numSymbols = htn->numTasks;
    to2s->numTerminals = htn->numActions;

    // initialize Ni sets
//    to2s->NumNis = htn->numCyclicSccs;
//    to2s->NiSize = new int[htn->numCyclicSccs];
//    to2s->Ni = new int *[htn->numCyclicSccs];
//    to2s->SymToNi = new int[to2s->numSymbols];
//    for (int i = 0; i < to2s->numSymbols; i++) {
//        to2s->SymToNi[i] = -1; // init as non-recursive
//    }
//
//    cout << "- collecting SCC data" << endl;
//    for (int k = 0; k < htn->numCyclicSccs; k++) {
//        int scc = htn->sccsCyclic[k];
//        to2s->NiSize[k] = htn->sccSize[scc];
//        to2s->Ni[k] = new int[to2s->NiSize[k]];
//        for (int j = 0; j < htn->sccSize[scc]; j++) {
//            to2s->Ni[k][j] = htn->sccToTasks[scc][j];
//            to2s->SymToNi[to2s->Ni[k][j]] = k;
//        }
//    }

    cout << "- adding methods as grammar rules" << endl;
    for (int iM = 0; iM < htn->numMethods; iM++) {
        vector<int> *rule = mToRule(htn, iM);
        to2s->addRule(rule);
    }
    cout << "Analysing rules" << endl;
    to2s->initDataStructures(htn->initialTask);
    to2s->calcSCCs(htn->initialTask);
    to2s->analyseRules(true);
    gettimeofday(&tp, NULL);
    endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timeHtnToGrammar=" << (endT - startT) << "]" << endl;
    startT = endT;

    if (!to2s->isRegurlar) {
        //to2s->printRules();
        CFtoRegGrammarEnc approx;
        cout << "- re-encode rules" << endl;
        approx.overapproximate(to2s, htn);
        cout << "- init data structures" << endl;
        to2s->initDataStructures(htn->initialTask);
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

    gettimeofday(&tp, NULL);
    long startB = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Building DFA" << endl;
    StdVectorFst* fa = to2s->makeFABU(htn, htn->initialTask);
    gettimeofday(&tp, NULL);
    long endB = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [buildingDFA=" << (endB - startB) << "]" << endl;
    //fa->showDOT();
    //fa->showDOT(htn->taskNames);
    string dFile2 = "domain.pddl";
    string pFile2 = "problem.sas";

    SASWriter mw2;
    mw2.write(htn, fa, dFile2, pFile2);

//    ModelWriter mw;
//    mw.write(htn, fa, dFile2, pFile2);

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

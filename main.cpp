#include <iostream>
#include "htnModel/Model.h"
#include "translation/CFGtoFDAtranslator.h"
#include "translation/CFtoRegGrammarEnc.h"
#include "SASWriter.h"
#include "ModelWriter.h"
#include "translation/StateBasedReachability.h"
#include <vector>
#include <cassert>
#include <sys/time.h>

vector<int> *mToRule(const Model *htn, int iM);

using namespace std;
using namespace progression;

const int TD = 0;
const int BU = 1;
const int NoOpt = 2;
const int PostOpt = 3;
const int InterOpt = 4;

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
    int algo = -1;
    int opt = -1;
    int inplaceThreshold = -1;

    bool printhelp = false;
    if (argc < 4) {
        printhelp = true;
    } else {
        string str = argv[1];
        if (str == "TD") {
            algo = TD;
            opt = NoOpt;
        } else if (str == "TD-PO") {
            algo = TD;
            opt = PostOpt;
        } else if (str == "BU-IO") {
            algo = BU;
            opt = InterOpt;
        }
        inplaceThreshold = atoi(argv[2]);
        s = argv[3];
        if (argc == 5) seed = atoi(argv[4]);
    }

    if (printhelp){
        cout << "usage:" << endl;
        cout << "toad [TD|TD-PO|BU-IO] inplaceThreshold pandagrounding [seed]" << endl;
        cout << "- TD: top down like in ICAPS version" << endl;
        cout << "- TD-PO: top down + post optimization" << endl;
        cout << "- TD-IO: top down + intermediate optimization" << endl;

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
    gettimeofday(&tp, NULL);

    long endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "- [timePrepareModel=" << (endT - startT) << "]" << endl;
    startT = endT;

    /*
    * Building grammar
    */
    cout << "Starting translation" << endl;
    CFGtoFDAtranslator *to2s = new CFGtoFDAtranslator();
    to2s->numSymbols = htn->numTasks;
    to2s->numTerminals = htn->numActions;

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

    if (!to2s->isRegular) {
        CFtoRegGrammarEnc approx;
        cout << "- re-encode rules" << endl;
        approx.overapproximate(to2s, htn);
        cout << "- init data structures" << endl;
        to2s->initDataStructures(htn->initialTask);
        cout << "- calc SCCs" << endl;
        to2s->calcSCCs(htn->initialTask);
        cout << "- re-analysing rules" << endl;
        to2s->analyseRules(true);
        gettimeofday(&tp, NULL);
        endT = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "- [timeCfgToRegTransf=" << (endT - startT) << "]" << endl;
        startT = endT;
    }

    /*
     * Build FA
     */
    long startB = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    cout << "Building DFA" << endl;
    StdVectorFst* fa;
    if (algo == TD) {
        bool interOpt = (opt == InterOpt);
        fa = to2s->makeFATD(htn, htn->initialTask, inplaceThreshold, interOpt);
    } else {
        assert(algo == BU);
        fa = to2s->makeFABU(htn, htn->initialTask);
    }
#ifndef NDEBUG
    Verify(*fa);
#endif

//    StateBasedReachability sbr;
//    sbr.statePruning(htn, fa);
//    cout << "- [aftersbp=" << fa->NumStates() << "]" << endl;

    if ((opt == PostOpt) || (opt == InterOpt)) {
        gettimeofday(&tp, NULL);
        long beginPO = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        int oldSize = 0;
        int optimizations = 0;
        while (oldSize != fa->NumStates()) {
            oldSize = fa->NumStates();
            StdVectorFst *fst2 = new StdVectorFst();
            fst2->SetProperties(kAcceptor, true);
            RmEpsilon(fa);
            Determinize(*fa, fst2);
            delete fa;
            fa = fst2;
            Minimize(fa);
            if (oldSize > fa->NumStates()) {
                optimizations++;
            }
        }
        cout << "- [roundsOfSuccOpt=" << optimizations << "]" << endl;
        gettimeofday(&tp, NULL);
        long endPO = tp.tv_sec * 1000 + tp.tv_usec / 1000;
        cout << "- [postOptimization=" << (endPO - beginPO) << "]" << endl;
    }
    cout << "- [numStatesFinal=" << fa->NumStates() << "]" << endl;
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

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

    for (int k = 0; k < htn->numCyclicSccs; k++) {
        int scc = htn->sccsCyclic[k];
        to2s->NiSize[k] = htn->sccSize[scc];
        to2s->Ni[k] = new int[to2s->NiSize[k]];
        for (int j = 0; j < htn->sccSize[scc]; j++) {
            to2s->Ni[k][j] = htn->sccToTasks[scc][j];
            to2s->SymToNi[to2s->Ni[k][j]] = k;
        }
    }

    // add methods as rules
    for (int iM = 0; iM < htn->numMethods; iM++) {
        vector<int> *rule = mToRule(htn, iM);
        to2s->addRule(rule);
    }
    to2s->analyseRules();
    int S = htn->initialTask;

    cout << endl << "Creating DFA..." << endl;
    cout << "- Starting symbol: " << S << endl;

    to2s->dfa->startState = to2s->dfa->stateID++;
    to2s->dfa->finalState = to2s->dfa->stateID++;
    to2s->makeFA(to2s->dfa->startState, S, to2s->dfa->finalState);
    cout << "- done!" << endl;

    cout << "creating output STRIPS model" << endl;
    string dFile = "/home/dh/Schreibtisch/temp3/pddl-models/domain01.pddl";
    string pFile ="/home/dh/Schreibtisch/temp3/pddl-models/problem01.pddl";

    ModelWriter mw;
    mw.write(htn, to2s->dfa, dFile, pFile);
    cout << "done!" << endl;

    //to2s->dfa.print(htn->taskNames, startState, finalState);

    exit(0);
/*
    int a = 0;
    int b = 1;
    int c = 2;
    int d = 3;
    int S = 4;
    int A = 5;
    int B = 6;

    vector<int> *Ni = new vector<int>;
    Ni->push_back(-1);
    Ni->push_back(-1);
    Ni->push_back(-1);
    Ni->push_back(-1);
    Ni->push_back(0);
    Ni->push_back(0);
    Ni->push_back(1);

    vector<int> *rule;
    rule = new vector<int>; // S -> Aa
    rule->push_back(S);
    rule->push_back(A);
    rule->push_back(a);
    nse->addRule(rule);

    rule = new vector<int>; // A -> SB
    rule->push_back(A);
    rule->push_back(S);
    rule->push_back(B);
    nse->addRule(rule);

    rule = new vector<int>; // A -> Bb
    rule->push_back(A);
    rule->push_back(B);
    rule->push_back(b);
    nse->addRule(rule);

    rule = new vector<int>; // B -> Bc
    rule->push_back(B);
    rule->push_back(B);
    rule->push_back(c);
    nse->addRule(rule);

    rule = new vector<int>; // B -> d
    rule->push_back(B);
    rule->push_back(d);
    nse->addRule(rule);

    cout << "- calculate recursive non-terminals (N bar)." << endl;
    for (int i = 0; i < htn->numCyclicSccs; i++) {
        int scc = htn->sccsCyclic[i];
        for (int j = 0; j < htn->sccSize[scc]; j++) {
            int task = htn->sccToTasks[scc][j];
            cout << scc << " " << htn->taskNames[task] << endl;
        }
    }




    // Nbar = {S, A, B}
    // N1 = {S, A} rec(N1) = left
    // N2 = {B}    rec(N2) = left

    vector<int> *N = new vector<int>;
    N->push_back(S);
    N->push_back(A);
    nse->Nisets.push_back(N);

    N = new vector<int>;
    N->push_back(B);
    nse->Nisets.push_back(N);

    nse->Ni = Ni;
    nse->recursion = new vector<int>;
    nse->recursion->push_back(nse->left);
    nse->recursion->push_back(nse->left);
    nse->makeFA(nse->stateID++, S, nse->stateID++);

    vector<string> *names = new vector<string>;
    names->push_back("a");
    names->push_back("b");
    names->push_back("c");
    names->push_back("d");
    names->push_back("S");
    names->push_back("A");
    names->push_back("B");
    nse->dfa.print(names);

    cout << "Here we are!" << endl;*/
    return 0;
}

vector<int> *mToRule(const Model *htn, int iM) {
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
                int s = htn->ordering[iM][o] + 1;
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
    return rule;
}

#include <iostream>
#include "Model.h"
#include "translation/FiniteAutomaton.h"
#include "translation/NotSelfEmbedding.h"
#include <vector>

using namespace std;
using namespace progression;

int main(int argc, char *argv[]) {
    std::cout << "Translating Total Order HTN model to STRIPS model." << std::endl;

#ifndef NDEBUG
    cout << "You have compiled the search engine without setting the NDEBUG flag. This will make it slow and should only be done for debug." << endl;
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
    Model* htn = new Model();
    htn->filename = s;
    htn->read(s);
    //assert(htn->isHtnModel);

    htn->calcSCCs();
	htn->calcSCCGraph();

    NotSelfEmbedding* nse = new NotSelfEmbedding();
    int a = 0;
    int b = 1;
    int c = 2;
    int d = 3;
    int S = 4;
    int A = 5;
    int B = 6;

    vector<int>* Ni = new vector<int>;
    Ni->push_back(-1);
    Ni->push_back(-1);
    Ni->push_back(-1);
    Ni->push_back(-1);
    Ni->push_back(0);
    Ni->push_back(0);
    Ni->push_back(1);

    vector<int>* rule;
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

    // Nbar = {S, A, B}
    // N1 = {S, A} rec(N1) = left
    // N2 = {B}    rec(N2) = left

    vector<int>* N = new vector<int>;
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

    vector<string>* names = new vector<string>;
    names->push_back("a");
    names->push_back("b");
    names->push_back("c");
    names->push_back("d");
    names->push_back("S");
    names->push_back("A");
    names->push_back("B");
    nse->fa.print(names);

    cout << "Here we are!" << endl;
    return 0;
}

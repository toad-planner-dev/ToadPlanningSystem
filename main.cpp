#include <iostream>
#include "Model.h"

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

    return 0;
}

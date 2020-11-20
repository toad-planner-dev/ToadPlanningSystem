//
// Created by dh on 08.11.20.
//

#include <fstream>
#include <cassert>
#include "TailRecAnalysis.h"
#include <map>

using namespace std;

void TailRecAnalysis::analyse(Model *htn, string filename) {
    string outFileName = filename;
    std::ofstream fOut(outFileName);

    vector<int[3]> orderingRels;

    for (int i = 0; i < htn->numMethods; i++) {
        vector<int> *rule = new vector<int>;

        // generate TO vector
        set<int> done;
        set<int> sts;
        for (int iST = 0; iST < htn->numSubTasks[i]; iST++) {
            sts.insert(iST);
        }

        while (!sts.empty()) {
            int first = -1;
            for (int iST : sts) {
                bool unconstrained = true;
                for (int o = 0; o < htn->numOrderings[i]; o += 2) {
                    int p = htn->ordering[i][o];
                    int s = htn->ordering[i][o + 1];
                    if ((s == iST) && (done.find(p) == done.end())) {
                        unconstrained = false;
                        break;
                    }
                }
                if (unconstrained) {
                    if (first < 0) {
                        first = iST;
                    } else {
                        cout << "ERROR: The method " << htn->methodNames[i]
                             << " is partially ordered. Only totally ordered problems are supported." << endl;
                        exit(-1);
                    }
                }
            }
            assert(first >= 0);
            sts.erase(first);
            done.insert(first);
            int st = htn->subTasks[i][first];
            rule->push_back(st);
        }

        int decompT = htn->decomposedTask[i];
        for(int j = 0; j < rule->size(); j++) {
            //orderingRels.
            rule->at(j);
        }
    }

    map<int, set<int>> xIsSecond; // mapping from x to rules where x is second

    fOut << "int";
    for(int i = 0; i < htn->numTasks; i++) {
        if (i > 0)
            fOut << ",";
        fOut << " t" << i;
    }
    fOut << ";" << endl;
    fOut.close();
}

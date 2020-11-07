//
// Created by Daniel Höller on 28.10.20.
//

#include <fstream>
#include <map>
#include "GroundVerifier.h"
#include <cassert>

void GroundVerifier::verify(progression::Model *htn, string sasPlan) {
    //
    // read plan
    //

    for(int i = 0; i < htn->numTasks; i++) {
        for(int j = i + 1; j < htn->numTasks; j++) {
            if(htn->taskNames[i] == htn->taskNames[j]) {
                cout << "Found two actions with same name" << endl;
                exit(-1);
            }
        }
    }

    std::ifstream fIn(sasPlan);
    std::string line;
    vector<int> prefix;
    set<int> distinctActions;
    StringUtil su;
    while (std::getline(fIn, line)) {
        if (line.rfind(';') == 0) continue;
        line = line.substr(1, line.length() - 2);
        bool found = false;
        for (int i = 0; i < htn->numTasks; i++) {
            if (line.compare(su.cleanStr(htn->taskNames[i])) == 0) {
                prefix.push_back(i);
                distinctActions.insert(i);
                found = true;
                break;
            }
        }
        if (!found) {
            cout << "task name not found: " << line << endl;
            exit(-1);
        }
    }
    fIn.close();

    set<int>* unreachableT = new set<int>;
    set<int>* unreachableM = new set<int>;
    set<int>* newlyUnrT = new set<int>;
    set<int>* newlyUnrM = new set<int>;
    for(int a = 0; a < htn->numActions; a++) {
        if (distinctActions.find(a) == distinctActions.end()) {
            newlyUnrT->insert(a);
            unreachableT->insert(a);
        }
    }
    while(!newlyUnrT->empty()) {
        for (int t : *newlyUnrT) {
            for (int mi = 0; mi < htn->stToMethodNum[t]; mi++) {
                int m = htn->stToMethod[t][mi];
                if (unreachableM->find(m) == unreachableM->end()) {
                    newlyUnrM->insert(m);
                    unreachableM->insert(m);
                }
            }
        }
        newlyUnrT->clear();
        for(int m : *newlyUnrM) {
            int t = htn->decomposedTask[m];
            bool oneleft = false;
            for (int mi = 0; mi < htn->numMethodsForTask[t]; mi++) {
                int m2 = htn->taskToMethods[t][mi];
                if (unreachableM->find(m2) == unreachableM->end()) {
                    oneleft = true;
                    break;
                }
            }
            if (!oneleft) {
                newlyUnrT->insert(t);
                unreachableT->insert(t);
            }
        }
        newlyUnrM->clear();
    }
    cout << "- pruned " << unreachableT->size() << " tasks and " << unreachableM->size() << " methods." << endl;

    // prepare things
    int numNewBits = prefix.size() + 2;

    // new task structure:
    // pppccccnnn
    // - p: copies of actions, one for each action in prefix (as many as there are actions in prefix)
    // - c: abstract tasks contained in original
    // - n: newly introduced abstract tasks (as many as there are distinct tasks in prefix)

    map<int,int> old2new;
    int current = prefix.size();
    for(int i = htn->numActions; i < htn->numTasks; i++) {
        if(unreachableT->find(i) == unreachableT->end()) {
            old2new[i] = current++;
        }
    }

    // mapping from actions contained in the prefix to the new abstract tasks
    map<int,int> prim2abs;
    for (int a : distinctActions) {
        prim2abs[a] = current++;
        old2new[a] = prim2abs[a];
    }

    //
    // encode problem
    //
    string outFileName = sasPlan.append(".verify");
    std::ofstream fOut(outFileName);
    fOut << ";; #state features" << endl;
    fOut << (htn->numStateBits + numNewBits) << endl;
    for (int i = 0; i < htn->numStateBits; i++) {
        fOut << htn->factStrs[i] << endl;
    }
    for (int i = 0; i < numNewBits; i++) {
        fOut << "prefpos" << i << endl;
    }

    fOut << endl << ";; Mutex Groups" << endl;
    fOut << (htn->numVars + 1) << endl;
    for (int i = 0; i < htn->numVars; i++) {
        fOut << htn->firstIndex[i] << " " << htn->lastIndex[i] << " " << htn->varNames[i] << endl;
    }
    int first = htn->lastIndex[htn->numVars - 1] + 1;
    int last = first + numNewBits - 1;
    fOut << first << " " << last << " prefixOrdering" << endl;

    fOut << endl << ";; further strict Mutex Groups" << endl;
    fOut << htn->numStrictMutexes << endl;
    for (int i = 0; i < htn->numStrictMutexes; i++) {
        fOut << htn->strictMutexes[i][0] << " " << htn->strictMutexes[i][1] << " -1" << endl;
    }

    fOut << endl << ";; further non strict Mutex Groups" << endl;
    fOut << htn->numMutexes << endl;
    for (int i = 0; i < htn->numMutexes; i++) {
        fOut << htn->mutexes[i][0] << " " << htn->mutexes[i][1] << " -1" << endl;
    }

    fOut << endl << ";; known invariants" << endl;
    fOut << htn->numInvariants << endl;
    for (int i = 0; i < htn->numInvariants; i++) {
        fOut << htn->invariants[i][0] << " " << htn->invariants[i][1] << " -1" << endl;
    }

    fOut << endl << ";; Actions" << endl;
    fOut << prefix.size() << endl;
    for (int i = 0; i < prefix.size(); i++) {
        writeAction(htn, fOut, prefix[i], first + i, first + i + 1);
    }

    fOut << endl << ";; initial state" << endl;
    for (int i = 0; i < htn->s0Size; i++) {
        fOut << htn->s0List[i] << " ";
    }
    fOut << first << " -1" << endl;

    fOut << endl << ";; goal" << endl;
    for (int i = 0; i < htn->gSize; i++) {
        fOut << htn->gList[i] << " ";
    }
    fOut << last - 1 << " -1" << endl;

    fOut << endl << ";; tasks (primitive and abstract)" << endl;
    int numTasks = htn->numTasks + prefix.size() - unreachableT->size();
    fOut << numTasks << endl;
    int check = 0;
    for (int i = 0; i < prefix.size(); i++) {
        fOut << "0 " << htn->taskNames[prefix[i]] << endl;
        check++;
    }
    for (int i = htn->numActions; i < htn->numTasks; i++) {
        if(unreachableT->find(i) == unreachableT->end()) {
            fOut << "1 " << htn->taskNames[i] << endl;
            check++;
        }
    }
    for (int t : distinctActions) {
        fOut << "1 <abs>" << htn->taskNames[t] << endl;
        check++;
    }
    assert(check == numTasks);

    fOut << endl << ";; initial abstract task" << endl;
    fOut << old2new[htn->initialTask] << endl;

    fOut << endl << ";; methods" << endl;
    int numMethods = htn->numMethods + prefix.size() - unreachableM->size();
    fOut << numMethods << endl;
    check = 0;
    for (int i = 0; i < htn->numMethods; i++) {
        if(unreachableM->find(i) == unreachableM->end()) {
            fOut << htn->methodNames[i] << endl;
            fOut << old2new[htn->decomposedTask[i]] << endl;
            for (int j = 0; j < htn->numSubTasks[i]; j++) {
                fOut << old2new[htn->subTasks[i][j]] << " ";
            }
            fOut << "-1" << endl;
            for (int j = 0; j < htn->numOrderings[i]; j++) {
                fOut << htn->ordering[i][j] << " ";
            }
            fOut << "-1" << endl;
            check++;
        }
    }

    // methods from new abstract tasks to prefix copies
    for(int i = 0; i < prefix.size(); i++) {
        int a = prefix[i];
        fOut <<  "__<method2pref" << i << ">" << htn->taskNames[a] << endl;
        fOut << prim2abs[a] << endl;
        fOut << i << " -1" << endl;
        fOut << "-1" << endl; // ordering relations
        check++;
    }
    assert(check == numMethods);

    fOut.close();
}

void GroundVerifier::writeAction(Model *htn, ofstream &fOut, int iAction, int pFrom, int pTo) {
    fOut << htn->actionCosts[iAction] << endl;
    for (int j = 0; j < htn->numPrecs[iAction]; j++) {
        fOut << htn->precLists[iAction][j] << " ";
    }
    fOut << pFrom << " "; // add precondition
    fOut << "-1" << endl;

    for (int j = 0; j < htn->numAdds[iAction]; j++) {
        fOut << "0 " << htn->addLists[iAction][j] << "  ";
    }
    fOut << "0 " << pTo << " "; // add add effect
    fOut << "-1" << endl;
    if ((htn->numConditionalAdds[iAction] > 0) || (htn->numConditionalDels[iAction] > 0)) {
        cout << "Conditional effects not supported" << endl;
        exit(-1);
    }

    for (int j = 0; j < htn->numDels[iAction]; j++) {
        fOut << "0 " << htn->delLists[iAction][j] << "  ";
    }
    fOut << "0 " << pFrom << " "; // add del effect
    fOut << "-1" << endl;
}

//
// Created by dh on 19.08.22.
//

#include "StateBasedReachability.h"

void StateBasedReachability::statePruning2(Model *htn, StdVectorFst *fst) {
    cout << "- expensive state-based pruning" << endl;
//    showDOT(fst);
    const int numStates = fst->NumStates();
    // initial one dr state per dfa state
    vector<bool> *drState = new vector<bool>[numStates];
    for (int i = 0; i < numStates; i++) {
        for (int j = 0; j < htn->numStateBits; j++) {
            drState[i].push_back(false);
        }
    }
    for (int i = 0; i < htn->s0Size; i++) {
        int f = htn->s0List[i];
        drState[0][f] = true;
    }

    vector<int> fringe;
    set<int> inFringe;
    fringe.push_back(0);
    inFringe.insert(0);
    while (!fringe.empty()) {
        int s = fringe.back();
        fringe.pop_back();
        inFringe.erase(s);
        for (ArcIterator<StdFst> aiter(*fst, s); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int a = arc.ilabel - 1;
            int s2 = arc.nextstate;
            bool applicable = true;
            for (int i = 0; i < htn->numPrecs[a]; i++) {
                int f = htn->precLists[a][i];
                if (!drState[s][f]) {
                    applicable = false;
                    break;
                }
            }
            bool changed = false;
            if (applicable) {
                for (int i = 0; i < htn->numAdds[a]; i++) {
                    int f = htn->addLists[a][i];
                    if (!drState[s2][f]) {
                        drState[s2][f] = true;
                        changed = true;
                    }
                }
                for (int f = 0; f < htn->numStateBits; f++) { // progress rest of state
                    if (drState[s][f] && !drState[s2][f]) {
                        drState[s2][f] = true;
                        changed = true;
                    }
                }
            }
            if (changed) {
                if (inFringe.find(s2) == inFringe.end()) {
                    fringe.push_back(s2);
                    inFringe.insert(s2);
                }
            }
        }
    }
    // pruning
    int prunedArcs = 0;
    int otherArcs = 0;
    vector<int> arcs;
    for (int s = 0; s < numStates; s++) {
        arcs.clear();
        for (ArcIterator<StdFst> aiter(*fst, s); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            int a = arc.ilabel - 1;
            bool applicable = true;
            for (int i = 0; i < htn->numPrecs[a]; i++) {
                int f = htn->precLists[a][i];
                if (!drState[s][f]) {
                    applicable = false;
                    break;
                }
            }
            if (applicable) {
                arcs.push_back(arc.ilabel - 1);
                arcs.push_back(arc.nextstate);
                otherArcs++;
            } else {
                prunedArcs++;
            }
        }
        fst->DeleteArcs(s);
        for (int i = 0; i < arcs.size(); i+=2) {
            addRule(fst, s, arcs[i], arcs[i + 1], 0);
        }
    }
    delete[] drState;
    cout << "- pruned " << prunedArcs << " of " << (prunedArcs + otherArcs) << " arcs." << endl;
//    showDOT(fst);
}

void StateBasedReachability::statePruning(Model *htn, StdVectorFst *fst) {
    const int numStates = fst->NumStates();
    set<int>* incommingActions = new set<int>[numStates];
    for (StateIterator<StdVectorFst> siter(*fst); !siter.Done(); siter.Next()) {
        int state_id = siter.Value();
        for (ArcIterator<StdFst> aiter(*fst, state_id); !aiter.Done(); aiter.Next()) {
            const StdArc &arc = aiter.Value();
            incommingActions[arc.nextstate].insert(arc.ilabel - 1);
        }
    }
    set<int>* accumState = new set<int>;
    set<int>* actionInv = new set<int>;
    vector<int> arcs;
    for (int state = 0; state < numStates; state++) {
        if (state == fst->Start()) {
            continue;
        }
        accumState->clear();
        bool first = true;
        if (!incommingActions[state].empty()) {
            for (int action : incommingActions[state]) {
                actionInv->clear();
                if (action != Epsilon) {
                    // preconditions that are not changed
                    for (int i = 0; i < htn->numPrecs[action]; i++) {
                        int prec = htn->precLists[action][i];
                        actionInv->insert(prec);
                    }
                    // effects
                    for (int i = 0; i < htn->numDels[action]; i++) {
                        int del = htn->delLists[action][i];
                        actionInv->erase(del);
                    }
                    for (int i = 0; i < htn->numAdds[action]; i++) {
                        int add = htn->addLists[action][i];
                        actionInv->insert(add);
                    }
                }
                if (first) {
                    first = false;
                    accumState->insert(actionInv->begin(), actionInv->end());
                } else { // intersect
                    for (auto iter = accumState->begin(); iter != accumState->end();) {
                        int p = *iter;
                        if (actionInv->find(p) == actionInv->end()) {
                            iter = accumState->erase(iter);
                        } else {
                            ++iter;
                        }
                    }
                }
            }
            if (!accumState->empty()) {
                arcs.clear();
                for (ArcIterator<StdFst> aiter(*fst, state); !aiter.Done(); aiter.Next()) {
                    const StdArc &arc = aiter.Value();
                    int action = arc.ilabel - 1;
                    bool applicable = true;
                    for (int i = 0; i < htn->numPrecs[action]; i++) {
                        int p = htn->precLists[action][i];
                        int var = htn->bitToVar[p];
                        for (int j = htn->firstIndex[var]; j <= htn->lastIndex[var]; j++) {
                            if ((j != p) && (accumState->find(j) != accumState->end())) {
                                applicable = false;
                                break;
                            }
                        }
                        if (!applicable) {
                            break;
                        }
                    }
                    if (applicable) {
                        arcs.push_back(arc.ilabel - 1);
                        arcs.push_back(arc.nextstate);
                    }
                }
                fst->DeleteArcs(state);
                for (int i = 0; i < arcs.size(); i+=2) {
                    addRule(fst, state, arcs[i], arcs[i + 1], 0);
                }
            }
        }
    }
//    pFa->delta->ensureBW(); // label -> (to -> from)
//    unordered_map<tStateID, set<int>*> pStates; // partial state that holds with certainty
//    for (auto l : *pFa->delta->backward) {
//        if(l.first == epsilon)
//            continue;
//        for (auto to : *l.second) {
//            if (pStates.find(to.first) == pStates.end()) {
//                set<int>* accumState = new set<int>;
//                pStates.insert({to.first, accumState});
//                int action = l.first;
//
//                // preconditions that are not changed
//                for (int i = 0; i < htn->numPrecs[action]; i++) {
//                    int prec = htn->precLists[action][i];
//                    accumState->insert(prec);
//                }
//                // effects
//                for (int i = 0; i < htn->numDels[action]; i++) {
//                    int del = htn->delLists[action][i];
//                    accumState->erase(del);
//                }
//                for (int i = 0; i < htn->numAdds[action]; i++) {
//                    int add = htn->addLists[action][i];
//                    accumState->insert(add);
//                }
//            } else {
//                set<int>* accumState = pStates.find(to.first)->second;
//                if (accumState == nullptr) {
//                    continue;
//                }
//                // intersect old set with (unchanged precs \cup add effects)
//                set<int>* tempPState = new set<int>;
//                int action = l.first;
//
//                // preconditions that are not changed
//                for (int i = 0; i < htn->numPrecs[action]; i++) {
//                    int prec = htn->precLists[action][i];
//                    tempPState->insert(prec);
//                }
//                // effects
//                for (int i = 0; i < htn->numDels[action]; i++) {
//                    int del = htn->delLists[action][i];
//                    tempPState->erase(del);
//                }
//                for (int i = 0; i < htn->numAdds[action]; i++) {
//                    int add = htn->addLists[action][i];
//                    tempPState->insert(add);
//                }
//                for(auto iter = accumState->begin(); iter != accumState->end(); ) {
//                    int p = *iter;
//                    if (tempPState->find(p) == tempPState->end()) {
//                        iter = accumState->erase(iter);
//                    } else {
//                        ++iter;
//                    }
//                }
//                delete tempPState;
//
//                if (accumState->empty()) { // closed
//                    delete accumState;
//                    pStates.at(to.first) = nullptr;
//                }
//            }
//        }
//    }
////    cout << endl;
////    for (auto accumState : pStates) {
////        if (accumState.second == nullptr) {
////            continue;
////        }
////        cout << accumState.first <<  ":";
////        for(int p : *accumState.second) {
////            cout << " " << htn->factStrs[p];
////        }
////         cout << endl;
////    }
////    cout << endl;
//
//    pFa->delta->ensureFW(); // from -> (label -> to)
//    for (auto iter2 = pFa->delta->forward->begin(); iter2 != pFa->delta->forward->end();) {
//        auto from = *iter2;
//        if (pFa->sInit.find(from.first) !=pFa->sInit.end()) {
//            iter2++;
//            continue;
//        }
//        if ((pStates.find(from.first) == pStates.end()) || (pStates.find(from.first)->second == nullptr)) {
//            iter2++;
//            continue;
//        }
//
//        set<int> accumState = *pStates.find(from.first)->second;
//
//        bool totallyEmpty = false;
//        for (auto iter = from.second->begin(); iter != from.second->end(); ) {
//            auto actions = *iter;
//            bool applicable = true;
//            int action = actions.first;
//            if(action == epsilon) {
//                iter++;
//                continue;
//            }
//            for (int i = 0; i < htn->numPrecs[action]; i++) {
//                int p = htn->precLists[action][i];
//                int var =  htn->bitToVar[p];
//                for (int j = htn->firstIndex[var]; j <= htn->lastIndex[var]; j++) {
//                    if ((j != p) && (accumState.find(j) != accumState.end())) {
//                        applicable = false;
//                        break;
//                    }
//                }
//                if (!applicable) {
//                    break;
//                }
//            }
//            if (!applicable) {
//                delete actions.second;
//                iter = from.second->erase(iter);
//                pFa->delta->numTransitions--;
//                if (from.second->empty()) {
//                    iter2 = pFa->delta->forward->erase(iter2);
//                    totallyEmpty = true;
//                    break;
//                }
//            } else {
//                iter++;
//            }
//        }
//        if (!totallyEmpty) {
//            iter2++;
//        }
//    }
//    for (auto accumState : pStates) {
//        delete accumState.second;
//    }
}

void StateBasedReachability::addRule(StdVectorFst *fst, int from, int label, int to, int costs) {
    fst->AddArc(from, StdArc(label + 1, label + 1, 0, to));
}

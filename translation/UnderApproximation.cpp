//
// Created by dh on 27.11.20.
//

#include "UnderApproximation.h"

void UnderApproximation::underapproximate(CFGtoFDAtranslator *g, Model *htn) {
    g->tempRules.clear();
    map<pair<int, int>, int> Qtop;
    map<pair<int, int>, int> Ql;
    map<pair<int, int>, int> Qr;
    map<pair<int, int>, int> Qlr;
    map<pair<int, int>, int> Qrl;
    map<pair<int, int>, int> Qbot;
    int d = 15;
    set<int> needToDelete;
    for (int i = 0; i < g->NumNis; i++) {
        if (g->NiRec[i] == g->recSelf) {
            // create new abstract tasks
            for (int j = 0; j < g->NiSize[i]; j++) {
                // (1)
                int A = g->Ni[i][j];
                needToDelete.insert(A);
                for (int f = 0; f < d; f++) {
                    Qtop.insert({{A, f}, g->numSymbols++});
                    Ql.insert({{A, f}, g->numSymbols++});
                    Qr.insert({{A, f}, g->numSymbols++});
                    Qlr.insert({{A, f}, g->numSymbols++});
                    Qrl.insert({{A, f}, g->numSymbols++});
                    Qbot.insert({{A, f}, g->numSymbols++});
                }

                // (2)
                g->tempRules.push_back(rOneToOne(A, Qtop[{A, 0}]));

                // (3)
                for (int f = 0; f < d; f++) {
                    g->tempRules.push_back(rOneToOne(Qtop[{A, 0}], Ql[{A, 0}]));
                    g->tempRules.push_back(rOneToOne(Qtop[{A, 0}], Qr[{A, 0}]));
                    g->tempRules.push_back(rOneToOne(Ql[{A, 0}], Qlr[{A, 0}]));
                    g->tempRules.push_back(rOneToOne(Qr[{A, 0}], Qrl[{A, 0}]));
                    g->tempRules.push_back(rOneToOne(Qlr[{A, 0}], Qbot[{A, 0}]));
                    g->tempRules.push_back(rOneToOne(Qrl[{A, 0}], Qbot[{A, 0}]));
                }

                for (int j = g->rFirst[A]; j <= g->rLast[A]; j++) {
                    vector<int> elemNi;
                    grRule *r = g->rules[j];

                    // which elements are in Ni?
                    for (int k = 0; k < r->rLength; k++) {
                        int B = r->right[k];
                        if (elem(g->Ni[i], g->NiSize[i], B)) {
                            elemNi.push_back(k);
                        }
                    }
                    if ((elemNi.size() == 1) && (elemNi[0] == 0)) { // first element
                        // (4)
                        for (int f = 0; f < d; f++) {
                            grRule *r2 = new grRule;
                            r2->left = Ql[{A, f}];
                            r2->rLength = r->rLength;
                            r2->right = new int[r2->rLength];
                            int B = r->right[0];
                            r2->right[0] = Ql[{B, f}];
                            for (int l = 1; l < r->rLength; l++) {
                                r2->right[l] = r->right[l];
                            }
                            g->tempRules.push_back(r2);

                            r2 = new grRule;
                            r2->left = Qrl[{A, f}];
                            r2->rLength = r->rLength;
                            r2->right = new int[r2->rLength];
                            r2->right[0] = Qrl[{B, f}];
                            for (int l = 1; l < r->rLength; l++) {
                                r2->right[l] = r->right[l];
                            }
                            g->tempRules.push_back(r2);
                        }
                    } else if ((elemNi.size() == 1) && (elemNi[0] == (r->rLength - 1))) { // last element
                        // (5)
                        for (int f = 0; f < d; f++) {
                            grRule *r2 = new grRule;
                            r2->left = Ql[{A, f}];
                            r2->rLength = r->rLength;
                            r2->right = new int[r2->rLength];
                            int B = r->right[r->rLength - 1];
                            r2->right[r->rLength - 1] = Ql[{B, f}];
                            for (int l = 0; l < r->rLength - 1; l++) {
                                r2->right[l] = r->right[l];
                            }
                            g->tempRules.push_back(r2);

                            r2 = new grRule;
                            r2->left = Qrl[{A, f}];
                            r2->rLength = r->rLength;
                            r2->right = new int[r2->rLength];
                            r2->right[r->rLength - 1] = Qrl[{B, f}];
                            for (int l = 0; l < r->rLength - 1; l++) {
                                r2->right[l] = r->right[l];
                            }
                            g->tempRules.push_back(r2);
                        }
                    } else if (elemNi.size() == 0) {
                        // (6) for m = 0
                        for (int f = 0; f < d; f++) {
                            grRule *r2 = new grRule;
                            r2->left = Qbot[{A, f}];
                            r2->rLength = r->rLength;
                            r2->right = new int[r2->rLength];
                            for (int l = 0; l < r->rLength; l++) {
                                r2->right[l] = r->right[l];
                            }
                            g->tempRules.push_back(r2);
                        }
                    } else {
                        // (6) for m > 0
                        for (int f = 0; f < (d - 1); f++) {
                            grRule *r2 = new grRule;
                            r2->left = Qbot[{A, f}];
                            r2->rLength = r->rLength;
                            r2->right = new int[r2->rLength];
                            for (int l = 0; l < r->rLength; l++) {
                                r2->right[l] = r->right[l];
                            }
                            for (int l = 0; l < elemNi.size(); l++) {
                                int iAi = elemNi[l];
                                int Ai = r->right[iAi];
                                r2->right[iAi] = Qtop[{Ai, f + 1}];
                            }
                            g->tempRules.push_back(r2);
                        }
                    }
                }
            }
        }
    }

    // remove old
    for (int i = 0; i < g->numRules; i++) {
        grRule *r = g->rules[i];
        if (needToDelete.find(r->left) == needToDelete.end()) {
            g->tempRules.push_back(r);
        }
    }
    delete[] g->rules;
    delete[] g->rFirst;
    delete[] g->rLast;

    g->initDataStructures();
}

grRule *UnderApproximation::rOneToOne(int from, int to) {
    return nullptr;
}

bool UnderApproximation::elem(int *pInt, int i, int i1) {
    return false;
}

grRule *UnderApproximation::rOneToEpsilon(int from) {
    return nullptr;
}


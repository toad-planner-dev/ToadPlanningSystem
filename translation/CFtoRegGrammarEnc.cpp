//
// Created by dh on 20.11.20.
//

#include <map>
#include "CFtoRegGrammarEnc.h"

void CFtoRegGrammarEnc::overapproximate(CFGtoFDAtranslator *g, Model *htn) {
    g->tempRules.clear();
    map<pair<int, int>, int> up;
    map<pair<int, int>, int> down;
    map<pair<int, int>, int> left;
    map<pair<int, int>, int> right;
    set<int> needToDelete;
    for (int i = 0; i < g->NumNis; i++) {
        if (g->NiRec[i] == g->recSelf) {
            for (int j = 0; j < g->NiSize[i]; j++) {
                int A = g->Ni[i][j];
                for (int k = 0; k < g->NiSize[i]; k++) {
                    int B = g->Ni[i][k];
                    needToDelete.insert(A);

                    up.insert({{A, B}, g->numSymbols++});
                    down.insert({{A, B}, g->numSymbols++});
                    left.insert({{A, B}, g->numSymbols++});
                    right.insert({{A, B}, g->numSymbols++});
                }
            }

            for (int j = 0; j < g->NiSize[i]; j++) {
                int A = g->Ni[i][j];
                // (2.1)
                g->tempRules.push_back(rOneToOne(A, up[{A, A}]));

                // (2.6)
                g->tempRules.push_back(rOneToEpsilon(left[{A, A}]));

                // (2.8)
                g->tempRules.push_back(rOneToEpsilon(right[{A, A}]));

                // (2.4)
                for (int k = 0; k < g->NiSize[i]; k++) {
                    int B = g->Ni[i][k];
                    g->tempRules.push_back(rOneToOne(down[{A, B}], right[{B, A}]));
                }
            }

            vector<int> R;
            for (int j = 0; j < g->NiSize[i]; j++) {
                int A = g->Ni[i][j];
                for (int k = g->rFirst[A]; k <= g->rLast[A]; k++) {
                    R.push_back(k);
                }
            }

            for (int ir : R) {
                grRule *r = g->rules[ir];
                vector<int> symFromNi;
                for (int k = 0; k < r->rLength; k++) {
                    int sym = r->right[k];
                    if (g->SymToNi[sym] == i) {
                        symFromNi.push_back(k);
                    }
                }

                // (2.2)
                if (symFromNi.size() == 0) {
                    int C = r->left;
                    for (int j = 0; j < g->NiSize[i]; j++) {
                        int A = g->Ni[i][j];
                        for (int k = 0; k < g->NiSize[i]; k++) {
                            int B = g->Ni[i][k];
                            grRule *rn = new grRule;
                            rn->left = up[{A, B}];
                            rn->rLength = r->rLength + 2;
                            rn->right = new int[r->rLength + 2];
                            for (int l = 0; l < r->rLength; l++) {
                                rn->right[l + 1] = r->right[l];
                            }
                            rn->right[0] = left[{A, C}];
                            rn->right[rn->rLength - 1] = down[{C, B}];
                            g->tempRules.push_back(rn);
                        }
                    }
                } else {
                    // (2.5)
                    int A = r->left;
                    int iC = symFromNi[0];
                    int C = r->right[iC];
                    for (int k = 0; k < g->NiSize[i]; k++) {
                        int B = g->Ni[i][k];
                        grRule *rn = new grRule;
                        rn->left = left[{A, B}];
                        rn->rLength = iC + 1;
                        rn->right = new int[rn->rLength];
                        for(int m = 0; m < iC; m++) {
                            rn->right[m] = r->right[m];
                        }
                        rn->right[iC] = left[{C, B}];
                        g->tempRules.push_back(rn);
                    }

                    // (2.7)
                    A = r->left;
                    iC = symFromNi[symFromNi.size() - 1];
                    C = r->right[iC];
                    for (int k = 0; k < g->NiSize[i]; k++) {
                        int B = g->Ni[i][k];
                        grRule *rn = new grRule;
                        rn->left = right[{A, B}];
                        rn->rLength = r->rLength - iC;
                        rn->right = new int[rn->rLength];
                        for(int m = iC + 1; m < r->rLength; m++) {
                            rn->right[m - iC] = r->right[m];
                        }
                        rn->right[0] = right[{C, B}];
                        g->tempRules.push_back(rn);
                    }

                    if (symFromNi.size() > 1) {
                        // (2.3)
                        for (int j = 0; j < g->NiSize[i]; j++) {
                            A = g->Ni[i][j];
                            for (int k = 0; k < g->NiSize[i]; k++) {
                                int B = g->Ni[i][k];
                                for(int l = 0; l < symFromNi.size() - 1; l++) {
                                    int iC = symFromNi[l];
                                    int iE = symFromNi[l + 1];
                                    int C = r->right[iC];
                                    int E = r->right[iE];

                                    grRule *rn = new grRule;
                                    rn->left = down[{A, B}];
                                    rn->rLength = iE - iC + 1;
                                    rn->right = new int[rn->rLength];
                                    for(int m = iC + 1; m < iE; m++) {
                                        rn->right[m - iC] = r->right[m];
                                    }
                                    rn->right[0] = right[{C, A}];
                                    rn->right[rn->rLength - 1] = up[{E, B}];
                                    g->tempRules.push_back(rn);
                                }
                            }
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
}

grRule *CFtoRegGrammarEnc::rOneToOne(int from, int to) {
    grRule *r = new grRule;
    r->left = from;
    r->right = new int[1]{to};
    r->rLength = 1;
    return r;
}

grRule * CFtoRegGrammarEnc::rOneToEpsilon(int from) {
    grRule *r = new grRule;
    r->left = from;
    r->right = new int[0];
    r->rLength = 0;
    return r;
}
;

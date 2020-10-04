//
// Created by dh on 03.10.20.
//

#ifndef TOSTRIPSAPPROXIMATION_MODELWRITER_H
#define TOSTRIPSAPPROXIMATION_MODELWRITER_H


#include "Model.h"

class ModelWriter {

public:
    ModelWriter(Model *model, string basicString, string basicString1);
    StringUtil su;

    Model* m;
    string dStr;
    string pStr;
    ofstream* dfile;
    ofstream* pfile;

    void writeAction(int action, int addP, int addAE, int addDE);
    void writePredDef(int maxState);

    void writeProblem(int startState,int goalState);

    void writeEpsilonAction(int i, int i1, int i2);
};


#endif //TOSTRIPSAPPROXIMATION_MODELWRITER_H

/*
 * HtnModel.h
 *
 *  Created on: 05.09.2017
 *      Author: Daniel Höller
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <climits>
#include <string>
#include <vector>
#include <set>
#include <forward_list>

#include "ProgressionNetwork.h"
#include "../utils/noDelIntSet.h"
#include "../utils/FlexIntStack.h"
#include "../utils/IntUtil.h"
#include "../utils/StringUtil.h"

using namespace std;

namespace progression {

class Model {
private:
	bool first = true;

	IntUtil iu;
	StringUtil su;

	int* readIntList(string s, int& size);
	tuple<int*,int*,int**> readConditionalIntList(string s, int& sizeA, int& sizeB, int*& sizeC);
	void generateMethodRepresentation();
	pair<planStep**, planStep**> initializeMethod(int method
#ifdef TRACESOLUTION
		, int parentSolutionStepIndex
#endif
			);
	int psID = 0;

	void printSummary();
	void printActions();
	void printAction(int i);
	void printMethods();

    void readClassical(std::istream& domainFile);
	void readHierarchical(std::istream& domainFile);
	void generateVectorRepresentation();

	void tarjan(int v);

	set<planStep*> potentiallyFirst;
	set<planStep*> done;
	forward_list<planStep*> potentialPredecessors;

#ifdef TRACKTASKSINTN
	void updateTaskCounterM(searchNode* n, searchNode* parent, int method);
	void updateTaskCounterA(searchNode* n, searchNode* parent, int action);
#endif

public:
	Model();
	virtual ~Model();
	void read(string f);
	void calcSCCs();
	searchNode* prepareTNi(const Model* htn);

	bool isHtnModel;
    string filename;

	// state-bits
	int numStateBits;
	string* factStrs;

	// variable definitions
	int numVars;
	int* firstIndex;
	int* lastIndex;
	string* varNames;
	int* bitToVar;

	// additional strict mutexes
	int numStrictMutexes;
	int** strictMutexes;
	int* strictMutexesSize;

	// additional mutexes
	int numMutexes;
	int** mutexes;
	int* mutexesSize;

	// invariants
	int numInvariants;
	int** invariants;
	int* invariantsSize;

	// action definitions
	int numActions;

	int* actionCosts;
	int** precLists;
	int** addLists;
	int** delLists;

	// dummy for CE
	int** conditionalAddLists;
	int** conditionalDelLists;
	
	int*** conditionalAddListsCondition;
	int*** conditionalDelListsCondition;
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
	bool* s0Vector;
	bool** addVectors;
	bool** delVectors;
#endif
	int numPrecLessActions;
	int* precLessActions;
	int* precToActionSize;
	int** precToAction;

	int* numPrecs;
	int* numAdds;
	int* numDels;
	
	int* numConditionalAdds;
	int* numConditionalDels;
	
	int** numConditionalAddsConditions;
	int** numConditionalDelsConditions;

	// s0 and goal
	int* s0List;
	int s0Size;
	int* gList;
	int gSize;

	// task definitions
	int numTasks;
	bool* isPrimitive;
	string* taskNames;

	// initial task
	int initialTask;

	// method definitions
	int numMethods;
	int* decomposedTask;
	int** subTasks;
	int* numSubTasks;
	int* numFirstPrimSubTasks;
	int* numFirstAbstractSubTasks;
	int** ordering; // this is a list of ints (p1,s2, p2,s2, ...) means that p1 is before s2, p2 before s2, ...
	int* numOrderings; // this is the length of the ARRAY, not the number of ordering constraints
	string* methodNames;
	int** methodsFirstTasks;
	int** methodSubtaskSuccNum;
	int* numFirstTasks;
	int** methodsLastTasks;
	int* numLastTasks;

	int** taskToMethods;
	int* numMethodsForTask;

	//For each method, two sorted arrays of ints are stored.
	// - the first one contains the task ids in ascending order
	// - the second one how often a task is contained in the subtasks
	int* numDistinctSTs = nullptr;
	int** sortedDistinctSubtasks = nullptr;
	int** sortedDistinctSubtaskCount = nullptr;

	// mapping from task to methods it is contained as subtasks
	int* stToMethodNum = nullptr;
	int** stToMethod = nullptr;

	// transition mechanics
	searchNode* decompose(searchNode *n, int taskNo, int method);
	searchNode* apply(searchNode *n, int taskNo);
	bool isApplicable(searchNode *n, int action) const;
	bool isGoal(searchNode *n) const;
#if (STATEREP == SRCALC1) || (STATEREP == SRCALC2)
	bool stateFeatureHolds(int f, searchNode* n) const;
#endif

	FlexIntStack *effectLess = nullptr;
	int numEffLessProg = 0;
#ifdef ONEMODMETH
	FlexIntStack *oneMod = nullptr;
#endif
	int numOneModActions = 0;
	int numOneModMethods = 0;


#ifdef MAINTAINREACHABILITY
	noDelIntSet intSet;
	void updateReachability(searchNode *n);
	void calcReachability(planStep *ps);
#endif

#ifdef MAINTAINREACHABILITYNOVEL
	int* taskCanBeReachedFromNum = nullptr;
	int** taskCanBeReachedFrom = nullptr;

	bool taskReachable(searchNode* tn, int t);
#endif



#ifdef CALCMINIMALIMPLIEDCOSTS
	int* minImpliedCosts;
	int* minImpliedDistance;
	void calcMinimalImpliedX();
#endif

	// permanent SCC information
	bool calculatedSccs = false;

	int numSCCs = -1;
	int* taskToSCC = nullptr;
	int** sccToTasks = nullptr;
	int* sccSize = nullptr;
	int sccMaxSize = -1;

	int numCyclicSccs = -1;
	int numSccOneWithSelfLoops = -1; // size one but with self-loops
	int* sccsCyclic = nullptr; // these may be sccs with size one but with a self loop, or sccs greater than one

	// SCC graph
	int* sccGnumSucc = nullptr;
	int* sccGnumPred = nullptr;
	int** sccG = nullptr;
	int** sccGinverse = nullptr;
	void calcSCCGraph();

	// reachability
	int* numReachable = nullptr;
	int** reachable = nullptr;

	void writeToPDDL(string dName, string pName);

        void printMethod(int i);

        void printTDG();
    };
}
#endif /* MODEL_H_ */

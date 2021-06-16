#ifndef HELPER_H
#define HELPER_H

#include "Graphs/ICFG.h"
#include "Graphs/PAG.h"

#include <vector>

using namespace SVF;
using namespace std;

vector<ICFGNode*> getAllICFGStartPoints(ICFG* icfg);

Value* getFirstOperandFromPAGNode(PAGNode* pNode);

#endif
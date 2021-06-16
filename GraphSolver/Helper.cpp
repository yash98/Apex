#include "Helper.hpp"

#include <vector>

vector<ICFGNode*> getAllICFGStartPoints(ICFG* icfg) {
	int counter = 1;
	vector<ICFGNode*> startingPoints;
	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
		ICFGNode* node = i->second;
		if (!node->hasIncomingEdge()) {
			unordered_set<NodeID> visited;
			// SVFUtil::outs() << "STARTPOINT COUNTER: " << counter << "\n";
			startingPoints.push_back(node);
			counter++;
		}
	}
	return startingPoints;
}


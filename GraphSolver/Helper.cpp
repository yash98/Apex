#include "Helper.hpp"

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

// Gets Value of first operand if it can otherwise nulltpr
Value* getFirstOperandFromPAGNode(PAGNode* pNode) {
	if (!pNode->hasValue()) return nullptr;
	const Value* pVal = pNode->getValue();
	const Instruction* pInst = SVFUtil::dyn_cast<Instruction>(pVal);

	if (pInst == nullptr) return nullptr; 
	if (pInst->getNumOperands() <= 1) return nullptr;
	Value* operand0 = pInst->getOperand(1);
	return operand0;
}

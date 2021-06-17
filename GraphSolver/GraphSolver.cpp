#include "SVF-FE/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-FE/PAGBuilder.h"

#include "Helper.hpp"
#include <vector>
#include <unordered_set>
#include <map>
#include <string>
#include <fstream>
#include <stdexcept>

using namespace SVF;
using namespace llvm;
using namespace std;

static map<uint, int> opCodeRelevantOperand;

unordered_set<string> getSeedVariableNamesFromFile(const char* filename) {
	ifstream F(filename);
	if (!F.is_open())
    {
        throw std::invalid_argument(string("Error opening file for reading: ") + string(filename));
    }

	string varName;
	unordered_set<string> seedVariables;

	while (getline(F, varName)) {
		seedVariables.insert(varName);
	}

	F.close();
	return seedVariables;
}

string isNodeOfInterest(ICFGNode* iNode, unordered_set<string>& seedVariables) {
	if (!IntraBlockNode::classof(iNode)) return "";
	const IntraBlockNode* ibNode = SVFUtil::dyn_cast<IntraBlockNode>(iNode);
	const Instruction* inst = ibNode->getInst();
	if (inst == nullptr) return "";

	uint opCode = inst->getOpcode();
	if (opCodeRelevantOperand.find(opCode) == opCodeRelevantOperand.end()) return "";
	int relevantOperand = opCodeRelevantOperand[opCode];

	if (relevantOperand < 0) {
		if (inst->hasName()) {
			unordered_set<string>::iterator foundAt = seedVariables.find(inst->getName().data());
			if (foundAt != seedVariables.end()) {
				return *foundAt;
			}
		}
	} else {
		Value* operand = inst->getOperand(relevantOperand);
		if (operand->hasName()) {
			unordered_set<string>::iterator foundAt = seedVariables.find(operand->getName().data());
			if (foundAt != seedVariables.end()) {
				return *foundAt;
			}
		}
	}
	return "";
}

bool forwardDfs(ICFGNode* iNode, int depth, int maxDepth, unordered_set<ICFGNode*>& visited, unordered_set<ICFGNode*>& usefulNodes, unordered_set<string>& variableOfInterestEncountered, unordered_set<string>& seedVariables) {
	if (depth == maxDepth) return false;
	// visited.insert(iNode);

	bool foundVOI = false;
	string voi = isNodeOfInterest(iNode, seedVariables);
	if (isNodeOfInterest(iNode, seedVariables) != "") {
		foundVOI = true;
		variableOfInterestEncountered.insert(voi);
	}

	for (ICFGEdge* iEdge : iNode->getOutEdges()) {
		ICFGNode* dst = iEdge->getDstNode();
		// if (visited.find(dst) == visited.end()) {
			bool childFoundVOI = forwardDfs(dst, depth+1, maxDepth, visited, usefulNodes, variableOfInterestEncountered, seedVariables);
			foundVOI = foundVOI | childFoundVOI;
		// } 
	}
	
	if (foundVOI) usefulNodes.insert(iNode);
	return foundVOI;
}

bool backwardDfs(ICFGNode* iNode, int depth, int maxDepth, unordered_set<ICFGNode*>& visited, unordered_set<ICFGNode*>& usefulNodes, unordered_set<string>& variableOfInterestEncountered, unordered_set<string>& seedVariables) {
	// 0 to maxDepth -1
	if (depth == maxDepth) return false;
	// visited.insert(iNode);

	bool foundVOI = false;
	string voi = isNodeOfInterest(iNode, seedVariables);
	if (isNodeOfInterest(iNode, seedVariables) != "") {
		foundVOI = true;
		variableOfInterestEncountered.insert(voi);
	}

	for (ICFGEdge* iEdge : iNode->getInEdges()) {
		ICFGNode* src = iEdge->getSrcNode();
		// if (visited.find(src) == visited.end()) {
			bool childFoundVOI = backwardDfs(src, depth+1, maxDepth, visited, usefulNodes, variableOfInterestEncountered, seedVariables);
			foundVOI = foundVOI | childFoundVOI;
		// }
	}
	
	if (foundVOI) usefulNodes.insert(iNode);
	return foundVOI;
}

void pruneICFGNodes(ICFG* icfg, unordered_set<string>& seedVariables) {
	SVFUtil::outs() << "Total number of Nodes: " << icfg->getTotalNodeNum() << "\n";
	map<ICFGNode*, unordered_set<ICFGNode*>> nodeOfInterestComponent;
	map<ICFGNode*, unordered_set<string>> varOfInterestInComponent;
	int counter = 1;
	int dfsMaxDepth = 29;
	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
		ICFGNode* iNode = i->second;
		if (isNodeOfInterest(iNode, seedVariables) != "") {
			SVFUtil::outs() << "Node of Interest: " << counter << "\n";
			SVFUtil::outs() << iNode->toString() << "\n";
			unordered_set<ICFGNode*> visited1;
			unordered_set<ICFGNode*> usefulNodes1;
			unordered_set<string> voie1;
			forwardDfs(iNode, 0, dfsMaxDepth, visited1, usefulNodes1, voie1, seedVariables);
			unordered_set<ICFGNode*> visited2;
			unordered_set<ICFGNode*> usefulNodes2;
			unordered_set<string> voie2;
			backwardDfs(iNode, 0, dfsMaxDepth, visited2, usefulNodes2, voie2, seedVariables);
			
			usefulNodes1.insert(usefulNodes2.begin(), usefulNodes2.end());
			voie1.insert(voie2.begin(), voie2.end());
			nodeOfInterestComponent[iNode] = usefulNodes1;
			varOfInterestInComponent[iNode] = voie1;
			counter++;
		}
	}

	SVFUtil::outs() << "Starting Maximal VOI components" << "\n";
	int maxNumVOIE = 0;
	for (map<ICFGNode*, unordered_set<string>>::iterator it = varOfInterestInComponent.begin(); it!=varOfInterestInComponent.end(); it++) {
		int componentNumVOI = it->second.size();
		if (maxNumVOIE < componentNumVOI) {
			maxNumVOIE = componentNumVOI;
		}
	}

	SVFUtil::outs() << "Max VOI in components: " << maxNumVOIE << "\n";
	unordered_set<ICFGNode*> nodesToKeep;
	for (map<ICFGNode*, unordered_set<string>>::iterator it = varOfInterestInComponent.begin(); it!=varOfInterestInComponent.end(); it++) {
		int componentNumVOI = it->second.size();
		if (maxNumVOIE == componentNumVOI) {
			unordered_set<ICFGNode*> component = nodeOfInterestComponent[it->first];
			SVFUtil::outs() << it->first << " ";
			for (string varName: it->second) SVFUtil::outs() << varName << " ";
			SVFUtil::outs() << component.size() << "\n";
			nodesToKeep.insert(component.begin(), component.end());
		}
	}

	SVFUtil::outs() << "Removal Started" << "\n";
	unordered_set<ICFGNode*> nodesToRemove;
	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
		ICFGNode* iNode = i->second;
		if (nodesToKeep.find(iNode) == nodesToKeep.end()) nodesToRemove.insert(iNode);
	}

	SVFUtil::outs() << "Number of Nodes to keep: " << nodesToKeep.size() << "\n";
	SVFUtil::outs() << "Number of Nodes to remove: " << nodesToRemove.size() << "\n";

	for (ICFGNode* iNode : nodesToRemove) {
		ICFGNode i = *iNode;
		unordered_set<ICFGEdge*> inEdgesToRemove;
		auto inEdges = iNode->getInEdges();
		inEdgesToRemove.insert(inEdges.begin(), inEdges.end());
		for (ICFGEdge* iEdge : inEdgesToRemove) {
				icfg->removeICFGEdge(iEdge);
		}
		unordered_set<ICFGEdge*> outEdgesToRemove;
		auto outEdges = iNode->getOutEdges();
		outEdgesToRemove.insert(outEdges.begin(), outEdges.end());
		for (ICFGEdge* iEdge : outEdgesToRemove) {
				icfg->removeICFGEdge(iEdge);
		}
		icfg->removeICFGNode(iNode);
	}
}

int main(int argc, char ** argv) {
	if (argc < 3) return 0;
	vector<std::string> moduleNameVec = {argv[1]};
	SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);
	/// Build Program Assignment Graph (PAG)
	PAGBuilder builder;
	PAG *pag = builder.build(svfModule);
	/// ICFG
	ICFG *icfg = pag->getICFG();
	/// Create Andersen's pointer analysis
    // Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
	/// Sparse value-flow graph (SVFG)
    // SVFGBuilder svfBuilder;
    // SVFG* svfg = svfBuilder.buildFullSVFGWithoutOPT(ander);

	opCodeRelevantOperand = {{Instruction::Alloca, -1}, {Instruction::Load, -1}, {Instruction::Store, 1}, {Instruction::GetElementPtr, -1}};

	unordered_set<string> seedVariables = getSeedVariableNamesFromFile(argv[2]);
	pruneICFGNodes(icfg, seedVariables);
	SVFUtil::outs() << "Dumping" << "\n";
	icfg->dump("ICFG-dfs29-useful-prune-exhaustive");

	return 0;
}
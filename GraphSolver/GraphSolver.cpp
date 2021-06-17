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

	if (inst->hasName()) {
		unordered_set<string>::iterator foundAt = seedVariables.find(inst->getName().data());
		if (foundAt != seedVariables.end()) {
			return *foundAt;
		}
	}

	if (inst == nullptr) return ""; 
	int operandNum = inst->getNumOperands();
	for (int i=0; i<operandNum; i++) {
		Value* operand = inst->getOperand(i);
		if (operand->hasName()) {
			unordered_set<string>::iterator foundAt = seedVariables.find(operand->getName().data());
			if (foundAt != seedVariables.end()) {
				return *foundAt;
			}
		}
	}
	return "";
}

void forwardDfs(ICFGNode* iNode, unordered_set<ICFGNode*>& visited, unordered_set<string>& variableOfInterestEncountered, unordered_set<string>& seedVariables) {
	visited.insert(iNode);
	string voi = isNodeOfInterest(iNode, seedVariables);
	if (isNodeOfInterest(iNode, seedVariables) != "") variableOfInterestEncountered.insert(voi);

	for (ICFGEdge* iEdge : iNode->getInEdges()) {
		ICFGNode* dst = iEdge->getDstNode();
		if (visited.find(dst) == visited.end()) {
			forwardDfs(dst, visited, variableOfInterestEncountered, seedVariables);
		} 
	}
}

void backwardDfs(ICFGNode* iNode, unordered_set<ICFGNode*>& visited, unordered_set<string>& variableOfInterestEncountered, unordered_set<string>& seedVariables) {
	visited.insert(iNode);
	string voi = isNodeOfInterest(iNode, seedVariables);
	if (isNodeOfInterest(iNode, seedVariables) != "") variableOfInterestEncountered.insert(voi);

	for (ICFGEdge* iEdge : iNode->getInEdges()) {
		ICFGNode* src = iEdge->getSrcNode();
		if (visited.find(src) == visited.end()) {
			backwardDfs(src, visited, variableOfInterestEncountered, seedVariables);
		} 
	}
}

void pruneICFGNodes(ICFG* icfg, unordered_set<string>& seedVariables) {
	SVFUtil::outs() << "Total number of Nodes: " << icfg->getTotalNodeNum() << "\n";
	map<ICFGNode*, unordered_set<ICFGNode*>> nodeOfInterestComponent;
	map<ICFGNode*, unordered_set<string>> varOfInterestInComponent;
	int counter = 1;
	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
		ICFGNode* iNode = i->second;
		if (isNodeOfInterest(iNode, seedVariables) != "") {
			SVFUtil::outs() << "Node of Interest: " << counter << "\n";
			SVFUtil::outs() << iNode->toString() << "\n";
			unordered_set<ICFGNode*> visited1;
			unordered_set<string> voie1;
			forwardDfs(iNode, visited1, voie1, seedVariables);
			// unordered_set<ICFGNode*> visited2;
			// unordered_set<string> voie2;
			// backwardDfs(iNode, visited2, voie2, seedVariables);
			
			// visited1.insert(visited2.begin(), visited2.end());
			// voie1.insert(voie2.begin(), voie2.end());
			nodeOfInterestComponent[iNode] = visited1;
			varOfInterestInComponent[iNode] = voie1;
			counter++;
		}
	}

	SVFUtil::outs() << "Maximal VOI components" << "\n";
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
		for (ICFGEdge* iEdge : iNode->getInEdges()) {
			icfg->removeICFGEdge(iEdge);
		}
		for (ICFGEdge* iEdge : iNode->getOutEdges()) {
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

	unordered_set<string> seedVariables = getSeedVariableNamesFromFile(argv[2]);
	pruneICFGNodes(icfg, seedVariables);
	SVFUtil::outs() << "Dumping" << "\n";
	// svfg->dump("reduced-SVFG-less");

	return 0;
}
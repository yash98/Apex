#include "SVF-FE/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SVF-FE/PAGBuilder.h"

#include "ReducedICFG.hpp"
#include "Helper.hpp"
#include <vector>
#include <unordered_set>
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

bool isNodeOfInterest(VFGNode* sNode, unordered_set<string>& seedVariables) {
	const ICFGNode* iNode = sNode->getICFGNode();
	if (!IntraBlockNode::classof(iNode)) return false;
	const IntraBlockNode* ibNode = SVFUtil::dyn_cast<IntraBlockNode>(iNode);
	const Instruction* sInst = ibNode->getInst();

	if (sInst->hasName()) {
		if (seedVariables.find(sInst->getName().data()) != seedVariables.end()) return true;
	}

	if (sInst == nullptr) return false; 
	int operandNum = sInst->getNumOperands();
	for (int i=0; i<operandNum; i++) {
		Value* operand = sInst->getOperand(i);
		if (operand->hasName()) {
			if (seedVariables.find(operand->getName().data()) != seedVariables.end()) return true;
		}
	}
	return false;
}

void forwardDfs(VFGNode* sNode, unordered_set<VFGNode*>& visited, unordered_set<VFGNode*>& nodesToKeep) {
	visited.insert(sNode);
	nodesToKeep.insert(sNode);
	for (VFGEdge* sEdge : sNode->getOutEdges()) {
		VFGNode* dst = sEdge->getDstNode();
		// Verify this condition
		if (visited.find(dst) == visited.end() && nodesToKeep.find(dst) == nodesToKeep.end()) {
			forwardDfs(dst, visited, nodesToKeep);
		} 
	}
}

void backwardDfs(VFGNode* sNode, unordered_set<VFGNode*>& visited, unordered_set<VFGNode*>& nodesToKeep) {
	visited.insert(sNode);
	nodesToKeep.insert(sNode);
	for (VFGEdge* sEdge : sNode->getInEdges()) {
		VFGNode* src = sEdge->getSrcNode();
		if (visited.find(src) == visited.end() && nodesToKeep.find(src) == nodesToKeep.end()) {
			backwardDfs(src, visited, nodesToKeep);
		} 
	}
}

void pruneSVFGNodes(SVFG* svfg, unordered_set<string>& seedVariables) {
	SVFUtil::outs() << "Total number of Nodes: " << svfg->getSVFGNodeNum() << "\n";
	unordered_set<VFGNode*> nodesToKeep;
	int counter = 1;
	for(VFG::iterator i = svfg->begin(); i != svfg->end(); i++) {
		VFGNode* sNode = i->second;
		if (isNodeOfInterest(sNode, seedVariables)) {
			SVFUtil::outs() << "Node of Interest: " << counter << "\n";
			SVFUtil::outs() << sNode->toString() << "\n";
			unordered_set<VFGNode*> visited1;
			forwardDfs(sNode, visited1, nodesToKeep);
			unordered_set<VFGNode*> visited2;
			backwardDfs(sNode, visited2, nodesToKeep);
			counter++;
		}
	}

	SVFUtil::outs() << "Removal Started" << "\n";
	unordered_set<VFGNode*> nodesToRemove;
	for(VFG::iterator i = svfg->begin(); i != svfg->end(); i++) {
		VFGNode* sNode = i->second;
		if (nodesToKeep.find(sNode) == nodesToKeep.end()) nodesToRemove.insert(sNode);
	}

	SVFUtil::outs() << "Number of Nodes to keep: " << nodesToKeep.size() << "\n";
	SVFUtil::outs() << "Number of Nodes to remove: " << nodesToRemove.size() << "\n";


	for (VFGNode* sNode : nodesToRemove) {
		if (nodesToRemove.find(sNode) != nodesToRemove.end()) {
			for (VFGEdge* sEdge : sNode->getInEdges()) {
				svfg->removeSVFGEdge(sEdge);
			}
			for (VFGEdge* sEdge : sNode->getOutEdges()) {
				svfg->removeSVFGEdge(sEdge);
			}
			svfg->removeSVFGNode(sNode);
		}
	}
}

int main(int argc, char ** argv) {
	// Hack to limit SVF's processing cmdline args to just first argument 
	int arg_num = 0;
	char **arg_value = new char*[2];
	std::vector<std::string> moduleNameVec;
	SVFUtil::processArguments(2, argv, arg_num, arg_value, moduleNameVec);
	cl::ParseCommandLineOptions(arg_num, arg_value, "Reduced Code Graph Generation\n");

	// TODO: Add command line input check later

	SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);
	/// Build Program Assignment Graph (PAG)
	PAGBuilder builder;
	PAG *pag = builder.build(svfModule);
	/// ICFG
	ICFG *icfg = pag->getICFG();
	/// Create Andersen's pointer analysis
    Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
	/// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder;
    SVFG* svfg = svfBuilder.buildFullSVFGWithoutOPT(ander);

	unordered_set<string> seedVariables = getSeedVariableNamesFromFile(argv[2]);
	pruneSVFGNodes(svfg, seedVariables);
	SVFUtil::outs() << "Dumping" << "\n";
	svfg->dump("reduced_SVFG");

	return 0;
}
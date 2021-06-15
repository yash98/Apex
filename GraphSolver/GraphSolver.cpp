#include "SVF-FE/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SABER/LeakChecker.h"
#include "SVF-FE/PAGBuilder.h"

# include "ReducedICFG.hpp"
#include <unordered_set>
#include <string>

using namespace SVF;
using namespace llvm;
using namespace std;

static llvm::cl::opt<std::string> InputFilename(cl::Positional, llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

void visitAllPaths(ICFGNode* node, ICFG* icfg, unordered_set<NodeID>& visited, int depth) {
	NodeID id = node->getId();
	visited.insert(id);
	if (!node->hasOutgoingEdge()) {
		SVFUtil::outs() << depth << "DEPTH: END POINT REACHED:" << node->toString() << "\n";
	} else {
		SVFUtil::outs() << depth << "DEPTH: " << node->toString() << "\n";
	}

	// Get Child Nodes
	for(ICFGEdge* edge : node->getOutEdges()){
		ICFGNode* dst = edge->getDstNode();
		NodeID dstID = dst->getId();
		if (visited.find(dstID) == visited.end()) {
			visitAllPaths(dst, icfg, visited, depth+1);
		} 
		// else {
		// 	SVFUtil::outs() << "ALREADY VISITED: " << dstID << "\n";
		// }
	}
	visited.erase(id);
}

void icfgFullDFS(ICFG* icfg) {
	int dfsCounter = 1;
	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
		ICFGNode* node = i->second;
		if (!node->hasIncomingEdge()) {
			unordered_set<NodeID> visited;
			SVFUtil::outs() << "ICFG DFS COUNTER: " << dfsCounter << "\n";
			visitAllPaths(node, icfg, visited, 0);
			dfsCounter++;
		}
	}
}


void getAllStartPoints(ICFG* icfg) {
	int counter = 1;
	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
		ICFGNode* node = i->second;
		if (!node->hasIncomingEdge()) {
			unordered_set<NodeID> visited;
			SVFUtil::outs() << "STARTPOINT COUNTER: " << counter << "\n";
			visitAllPaths(node, icfg, visited, 0);
			counter++;
		}
	}
}

int main(int argc, char ** argv) {

	int arg_num = 0;
	char **arg_value = new char*[argc];
	std::vector<std::string> moduleNameVec;
	SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
	cl::ParseCommandLineOptions(arg_num, arg_value, "Reduced Code Graph Generation\n");

	SVFUtil::outs() << arg_num << " " << arg_value[1] << "\n";

	SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

	/// Build Program Assignment Graph (PAG)
	PAGBuilder builder;
	PAG *pag = builder.build(svfModule);
	// dump pag
	// pag->dump(svfModule->getModuleIdentifier() + ".pag");
	/// ICFG
	ICFG *icfg = pag->getICFG();
	// dump icfg
	// icfg->dump(svfModule->getModuleIdentifier() + ".icfg");

	// // iterate each ICFGNode on ICFG
	// for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++)
	// {
	// 	ICFGNode *n = i->second;
	// 	SVFUtil::outs() << n->toString() << "\n";
	// 	for(ICFGEdge* edge : n->getOutEdges()){
	// 		SVFUtil::outs() << edge->toString() << "\n";
	// 	}
	// }

	// // iterate each PAGNode on PAG
	// for(PAG::iterator p = pag->begin(); p != pag->end();p++)
	// {
	//     PAGNode *n = p->second;
	//     SVFUtil::outs() << n->toString() << "\n";
	//     for(PAGEdge* edge : n->getOutEdges()){
	//         SVFUtil::outs() << edge->toString() << "\n";
	//     }
	// }



	return 0;
}

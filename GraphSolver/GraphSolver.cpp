#include "SVF-FE/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SABER/LeakChecker.h"
#include "SVF-FE/PAGBuilder.h"

#include <unordered_set>

using namespace SVF;
using namespace llvm;
using namespace std;

static llvm::cl::opt<std::string> InputFilename(cl::Positional, llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

void icfgDFS(NodeID id, ICFG* icfg, unordered_set<NodeID> visited) {
	visited.insert(id);
	ICFGNode* node = icfg->getICFGNode(id);

	// Get Child Nodes
	int numEdges = 0;
	for(ICFGEdge* edge : node->getOutEdges()){
		NodeID dst = edge->getDstID();
		if !(visited.contains(id)) {
			icfgDFS(dst, icfg, visited);
			numEdges++;
		}
	}
	if (numEdges == 0) {

	} 
}

int main(int argc, char ** argv) {

	int arg_num = 0;
	char **arg_value = new char*[argc];
	std::vector<std::string> moduleNameVec;
	SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
	cl::ParseCommandLineOptions(arg_num, arg_value, "Reduced Code Graph Generation\n");

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

	unordered_set<NodeID> visited;

	return 0;
}

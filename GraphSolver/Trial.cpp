#include "SVF-FE/LLVMUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "SABER/LeakChecker.h"
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

static llvm::cl::opt<std::string> InputFilename(cl::Positional, llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));

// void visitAllPaths(ICFGNode* node, ICFG* icfg, unordered_set<NodeID>& visited, int depth) {
// 	NodeID id = node->getId();
// 	visited.insert(id);
// 	if (!node->hasOutgoingEdge()) {
// 		SVFUtil::outs() << depth << "DEPTH: END POINT REACHED:" << node->toString() << "\n";
// 	} else {
// 		SVFUtil::outs() << depth << "DEPTH: " << node->toString() << "\n";
// 	}

// 	// Get Child Nodes
// 	for(ICFGEdge* edge : node->getOutEdges()){
// 		ICFGNode* dst = edge->getDstNode();
// 		NodeID dstID = dst->getId();
// 		if (visited.find(dstID) == visited.end()) {
// 			visitAllPaths(dst, icfg, visited, depth+1);
// 		} 
// 		// else {
// 		// 	SVFUtil::outs() << "ALREADY VISITED: " << dstID << "\n";
// 		// }
// 	}
// 	visited.erase(id);
// }

// void icfgFullDFS(ICFG* icfg) {
// 	int dfsCounter = 1;
// 	for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
// 		ICFGNode* node = i->second;
// 		if (!node->hasIncomingEdge()) {
// 			unordered_set<NodeID> visited;
// 			SVFUtil::outs() << "ICFG DFS COUNTER: " << dfsCounter << "\n";
// 			visitAllPaths(node, icfg, visited, 0);
// 			dfsCounter++;
// 		}
// 	}
// }

// unordered_set<string> getSeedVariableNamesFromFile(const char* filename) {
// 	ifstream F(filename);
// 	if (!F.is_open())
//     {
//         throw std::invalid_argument(string("Error opening file for reading: ") + string(filename));
//     }

// 	string varName;
// 	unordered_set<string> seedVariables;

// 	while (getline(F, varName)) {
// 		seedVariables.insert(varName);
// 	}

// 	F.close();
// 	return seedVariables;
// }

// unordered_set<Value*> extendSeedVariables(PAG* pag, PointerAnalysis* pta, unordered_set<string> seedVariables) {
// 	unordered_set<Value*> extendedSeedVariableValues;
// 	for (PAG::iterator p = pag->begin(); p != pag->end(); p++) {
// 		PAGNode* pNode = p->second;
// 		SVFUtil::outs() << pNode->toString() << "\n";
// 		if (!pNode->hasValue()) continue;
// 		const Value* pVal = pNode->getValue();
// 		const Instruction* inst = SVFUtil::dyn_cast<Instruction>(pVal);
// 		if (inst == nullptr) continue; 
// 		if (inst->hasName()) SVFUtil::outs() << inst->getName() << "; ";
// 		SVFUtil::outs() << inst->getOpcodeName() << ", ";
// 		int opnt_cnt = inst->getNumOperands();
//         for(int i=0; i < opnt_cnt; i++)
//         {
// 			Value *opnd = inst->getOperand(i);
// 			if (opnd->hasName()) {
// 				string o = opnd->getName().data();
// 				SVFUtil::outs() << o << " " << opnd << ", " ;
// 			} else {
// 				SVFUtil::outs() << "ptr" << opnd << ", ";
// 			}
//         }
// 		SVFUtil::outs() << "\n";
// 		continue;
// 		Value* operand0 = getFirstOperandFromPAGNode(pNode);
// 		if (operand0 == nullptr) continue;

// 		if (operand0->hasName()) {
// 			string name = operand0->getName().data();
// 			if (seedVariables.find(name) != seedVariables.end()) {
// 				if (extendedSeedVariableValues.find(operand0) == extendedSeedVariableValues.end()) {
// 					extendedSeedVariableValues.insert(operand0);
// 					// REMOVE
// 					SVFUtil::outs() << "SEED FOUND" << "\n";
// 					if (operand0->hasName()) {
// 						string o = operand0->getName().data();
// 						SVFUtil::outs() << " " << o << " " << operand0 << "\n";
// 					} else {
// 						SVFUtil::outs() << " ptr" << operand0 << "\n";
// 					}
// 					SVFUtil::outs() << pNode->toString() << "\n";

// 				}
// 				const NodeBS& pts = pta->getPts(pNode->getId());

// 				for (NodeBS::iterator it = pts.begin(); it != pts.end(); it++) {
// 					PAGNode* targetObj = pta->getPAG()->getPAGNode(*it);

// 					Value* targetObjOperand0 = getFirstOperandFromPAGNode(targetObj);
// 					if (targetObjOperand0 == nullptr) continue;

// 					// REMOVE
// 					SVFUtil::outs() << "PTS FOUND" << "\n";
// 					if (targetObjOperand0->hasName()) {
// 						string o = targetObjOperand0->getName().data();
// 						SVFUtil::outs() << " " << o << " " << targetObjOperand0 << "\n";
// 					} else {
// 						SVFUtil::outs() << " ptr" << targetObjOperand0 << "\n";
// 					}
// 					SVFUtil::outs() << targetObj->toString() << "\n";
					

// 					if (extendedSeedVariableValues.find(targetObjOperand0) == extendedSeedVariableValues.end()) extendedSeedVariableValues.insert(targetObjOperand0);
// 				}
// 			}
// 		}
// 	}

// 	return extendedSeedVariableValues;
// }

// int main(int argc, char ** argv) {
// 	// Hack to limit SVF's processing cmdline args to just first argument 
// 	int arg_num = 0;
// 	char **arg_value = new char*[2];
// 	std::vector<std::string> moduleNameVec;
// 	SVFUtil::processArguments(2, argv, arg_num, arg_value, moduleNameVec);
// 	cl::ParseCommandLineOptions(arg_num, arg_value, "Reduced Code Graph Generation\n");

// 	// TODO: Add command line input check later

// 	SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

// 	/// Build Program Assignment Graph (PAG)
// 	PAGBuilder builder;
// 	PAG *pag = builder.build(svfModule);
// 	/// ICFG
// 	ICFG *icfg = pag->getICFG();
// 	Andersen* ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

// 	unordered_set<string> seedVariables = getSeedVariableNamesFromFile(argv[2]);
// 	// vector<ICFGNode*> startingPoints = getAllICFGStartPoints(icfg);
// 	unordered_set<Value*> extendedSeedVariableValues = extendSeedVariables(pag, ander, seedVariables);

// 	// for (Value* v: extendedSeedVariableValues) {
// 	// 	if (v->hasName()) {
//     //         string o = v->getName().data();
//     //         SVFUtil::outs() << " " << o << " " << v << "\n";
//     //       } else {
//     //         SVFUtil::outs() << " ptr" << v << "\n";
//     //       }
// 	// }

// 	return 0;
// }

#ifndef REDUCED_ICFG_H
#define REDUCED_ICFG_H

#include "Util/BasicTypes.h"
#include "Util/SVFUtil.h"
#include "Graphs/GenericGraph.h"

namespace SVF 
{

class ReducedICFGNode;

typedef GenericEdge<ReducedICFGNode> GenericReducedICFGEdgeTy;
class ReducedICFGEdge : public GenericReducedICFGEdgeTy
{
public:
	// /// Constructor
    // ReducedICFGEdge(ReducedICFGNode* s, ReducedICFGNode* d, GEdgeFlag k) : GenericReducedICFGEdgeTy(s,d,k)
    // {
    // }
    /// Constructor
    ReducedICFGEdge(ReducedICFGNode* s, ReducedICFGNode* d) : GenericReducedICFGEdgeTy(s,d,0)
    {
    }
    // /// Destructor
    // ~ReducedICFGEdge()
    // {
    // }
};

typedef GenericNode<ReducedICFGNode, ReducedICFGEdge> GenericReducedICFGNodeTy;
class ReducedICFGNode : public GenericReducedICFGNodeTy
{
public:
	NodeID correspondingICFGNodeID;

	// /// Constructor
    // ReducedICFGNode(NodeID i, GNodeK k, NodeID cid) : GenericReducedICFGNodeTy(i, k), correspondingICFGNodeID(cid)
    // {
    // }

    /// Constructor
    ReducedICFGNode(NodeID i, NodeID cid) : GenericReducedICFGNodeTy(i, 0), correspondingICFGNodeID(cid)
    {
    }
};

typedef GenericGraph<ReducedICFGNode,ReducedICFGEdge> GenericReducedICFGTy;
class ReducedICFG : public GenericReducedICFGTy
{
public:
	// /// Constructor
    // ReducedICFG();

	/// Dump graph into dot file
    void dump(const std::string& file, bool simple = false);
};

}

namespace llvm
{
/* !
 * GraphTraits specializations for generic graph algorithms.
 * Provide graph traits for traversing from a constraint node using standard graph traversals.
 */
template<> struct GraphTraits<SVF::ReducedICFGNode*> : public GraphTraits<SVF::GenericNode<SVF::ReducedICFGNode,SVF::ReducedICFGEdge>*  >
{
};

/// Inverse GraphTraits specializations for call graph node, it is used for inverse traversal.
template<>
struct GraphTraits<Inverse<SVF::ReducedICFGNode *> > : public GraphTraits<Inverse<SVF::GenericNode<SVF::ReducedICFGNode,SVF::ReducedICFGEdge>* > >
{
};

template<> struct GraphTraits<SVF::ReducedICFG*> : public GraphTraits<SVF::GenericGraph<SVF::ReducedICFGNode,SVF::ReducedICFGEdge>* >
{
    typedef SVF::ReducedICFGNode *NodeRef;
};

} // End namespace llvm

#endif

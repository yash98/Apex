#include "ReducedICFG.hpp"

using namespace SVF;
using namespace SVFUtil;

void ReducedICFG::dump(const std::string& file, bool simple)
{
    GraphPrinter::WriteGraphToFile(outs(), file, this, simple);
}

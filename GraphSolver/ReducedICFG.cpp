#include "Graphs/GraphPrinter.h"
#include "ReducedICFG.hpp"

using namespace SVF;

void ReducedICFG::dump(const std::string& file, bool simple)
{
    GraphPrinter::WriteGraphToFile(outs(), file, this, simple);
}

#ifndef CANGJIE_CHIR_SPLIT_CRITICAL_EDGES_H
#define CANGJIE_CHIR_SPLIT_CRITICAL_EDGES_H

#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/CHIR/IR/CHIRBuilder.h"

namespace Cangjie{
namespace CHIR{

class SplitCriticalEdges{
public:

   explicit SplitCriticalEdges(CHIRBuilder& builder) : builder(builder) {}
   bool Run(Function& func);

private:
   CHIRBuilder& builder;
   Block* SplitCriticalEdge(Block* origin, Block* dest, size_t edge);

};

} //namespace CHIR
} //namespace CHIR



#endif

#include "cangjie/CHIR/Optimization/SplitCriticalEdges.h"
#include "cangjie/CHIR/IR/Expression/Terminator.h"
#include "cangjie/CHIR/Utils/CHIRCasting.h"

using namespace Cangjie::CHIR;

bool SplitCriticalEdges::Run(Function& func) {
    bool changed = false;
    
    // Copies the original block list, for we are creating new ones
    auto originalBlocks = func.GetBody()->GetBlocks();
    

    for (Block* block : originalBlocks) {

        size_t numSuccessors = block->GetSuccessors().size();
         
        for(size_t i = 0 ; i < numSuccessors; ++i){
            Block* succ = block->GetSuccessors()[i];
            Block* dummy = SplitCriticalEdge(block, succ, i);
            if(dummy != nullptr) {
               changed = true; 
            }
        }
    }
    return changed;
}

Block* SplitCriticalEdges::SplitCriticalEdge(Block* origin, Block* dest, size_t edge) {
    // 1. Verifies if the edge is critical
    if (origin->GetSuccessors().size() <= 1 || dest->GetPredecessors().size() <= 1) {
        return nullptr; 
    }

    // 2. Creates Dummy Block and adds the Goto to the destination
    Block* dummyBlock = builder.CreateBlock(origin->GetParentBlockGroup());

    auto termGoTo = builder.CreateTerminator<GoTo>(dest, dummyBlock);
    dummyBlock->AppendExpression(termGoTo);

    // 3. Gets the current terminator from origin and changes its destination 
    auto terminator = origin->GetTerminator();
    CJC_NULLPTR_CHECK(terminator);

    terminator->ReplaceSuccessor(edge, *dummyBlock);

    
    return dummyBlock;
}

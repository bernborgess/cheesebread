#include "cangjie/CHIR/Optimization/SplitCriticalEdges.h"
#include "cangjie/CHIR/IR/Expression/Terminator.h"
#include "cangjie/CHIR/Utils/CHIRCasting.h"

using namespace Cangjie::CHIR;

bool SplitCriticalEdges::Run(Function& func) {
    bool changed = false;
    
    // Copia a lista original de blocos porque vamos criar novos
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
    // 1. Verifica se é aresta crítica
    if (origin->GetSuccessors().size() <= 1 || dest->GetPredecessors().size() <= 1) {
        return nullptr; 
    }

    // 2. Cria o Dummy Block e adiciona o Goto para o destino
    Block* dummyBlock = builder.CreateBlock(origin->GetParentBlockGroup());

    auto termGoTo = builder.CreateTerminator<GoTo>(dest, dummyBlock);
    dummyBlock->AppendExpression(termGoTo);

    // 3. Pega o terminador atual do origin e troca o destino
    auto terminator = origin->GetTerminator();
    CJC_NULLPTR_CHECK(terminator);

    terminator->ReplaceSuccessor(edge, *dummyBlock);

    
    return dummyBlock;
}

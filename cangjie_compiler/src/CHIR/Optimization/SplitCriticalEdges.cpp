#include "cangjie/CHIR/Optimization/SplitCriticalEdges.h"
#include "cangjie/CHIR/IR/Expression/Terminator.h"
#include "cangjie/CHIR/Utils/CHIRCasting.h"

using namespace Cangjie::CHIR;

bool SplitCriticalEdges::Run(Function& func) {
    bool changed = false;
    
    // Copia a lista original de blocos porque vamos criar novos
    auto originalBlocks = func.GetBody()->GetBlocks();
    

    for (Block* block : originalBlocks) {
        auto successors = block->GetSuccessors();
        for (Block* succ : successors) {
            Block* dummy = SplitCriticalEdge(block, succ);
            if (dummy != nullptr) {
                changed = true;
            }
        }
    }
    return changed;
}

Block* SplitCriticalEdges::SplitCriticalEdge(Block* origin, Block* dest) {
    // 1. Verifica se é aresta crítica
    if (origin->GetSuccessors().size() <= 1 || dest->GetPredecessors().size() <= 1) {
        return nullptr; 
    }

    // 2. Cria o Dummy Block e adiciona o Goto para o destino
    Block* dummyBlock = builder.CreateBlock(origin->GetParentBlockGroup());
    auto termGoTo = builder.CreateTerminator<GoTo>(dest, dummyBlock);
    dummyBlock->AppendExpression(termGoTo);

    // 3. Pega o terminador atual do origin e troca o destino
    auto expressions = origin->GetExpressions();
    auto terminator = StaticCast<Terminator*>(expressions.back());
    terminator->ReplaceSuccessor(*dest, *dummyBlock);

    
    return dummyBlock;
}

#ifndef COMPETITION_SSA_BUILDER_H
#define COMPETITION_SSA_BUILDER_H

#include "cangjie/Competition/DominatorTree.h"

namespace Competition {

// We wrap your ir::Block in a DJNode to hold the state required
// by Algorithm 4.1 from the Sreedhar-Gao paper.
struct DJNode {
    Block* block = nullptr;
    int level = 0;

    // Algorithm state flags
    bool visited = false;
    bool alpha = false;
    bool in_phi = false;
};

class SSABuilder {
public:
    explicit SSABuilder(const DominatorTree& domTree) : domTree_(domTree)
    {
    }

    // Computes the iterated dominance frontier (IDF) for a set of blocks.
    // N_alpha is the set of blocks that define a specific variable.
    // Returns the set of blocks where phi-functions must be inserted.
    std::vector<Block*> PlacePhiNodes(const std::vector<Block*>& n_alpha, Block* entryBlock);

    // Call this once before placing phis to precompute levels using the DomTree
    void ComputeLevels(Block* entryBlock);

private:
    const DominatorTree& domTree_;
    std::unordered_map<Block*, DJNode> djNodes_;
    int maxLevel_ = 0;

    DJNode* GetDJNode(Block* block);

    void ResetState();
};

} // namespace competition

#endif

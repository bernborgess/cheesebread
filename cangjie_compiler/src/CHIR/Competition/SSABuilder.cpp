#include "cangjie/Competition/SSABuilder.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Competition {

// Computes the iterated dominance frontier (IDF) for a set of blocks.
// N_alpha is the set of blocks that define a specific variable.
// Returns the set of blocks where phi-functions must be inserted.
std::vector<Block*> SSABuilder::PlacePhiNodes(const std::vector<Block*>& n_alpha, Block* entryBlock)
{
    std::vector<Block*> idf;
    if (n_alpha.empty())
        return idf;

    // 1. Initialize state
    ResetState();
    ComputeLevels(entryBlock);

    // 2. Setup PiggyBank
    // The PiggyBank is an array of lists of nodes, indexed by level.
    std::vector<std::vector<DJNode*>> piggyBank((size_t)(maxLevel_ + 1));
    int currentLevel = 0;

    // Helper lambda to insert into PiggyBank
    auto InsertNode = [&](DJNode* node) {
        piggyBank[(size_t)node->level].push_back(node);
        currentLevel = std::max(currentLevel, node->level);
    };

    // Helper lambda to get the highest level node from PiggyBank
    auto GetNode = [&]() -> DJNode* {
        while (currentLevel >= 0) {
            if (!piggyBank[(size_t)currentLevel].empty()) {
                DJNode* node = piggyBank[(size_t)currentLevel].back();
                piggyBank[(size_t)currentLevel].pop_back();
                return node;
            }
            currentLevel--;
        }
        return nullptr;
    };

    // 3. Initialize PiggyBank with N_alpha
    for (Block* b : n_alpha) {
        DJNode* node = GetDJNode(b);
        node->alpha = true;
        InsertNode(node);
    }

    // 4. Main Loop: Process nodes from the PiggyBank
    DJNode* currentRoot = nullptr;

    // Recursive lambda to visit nodes in the dominator sub-tree
    auto Visit = [&](auto& self, DJNode* x) -> void {
        // Check all CFG successors of x
        for (Block* succBlock : x->block->GetSuccessors()) {
            DJNode* y = GetDJNode(succBlock);

            // Determine edge type. If x is NOT the immediate dominator of y,
            // it is a J-edge. Otherwise, it is a D-edge.
            bool isJEdge = (domTree_.GetImmediateDominator(y->block) != x->block);

            if (isJEdge) {
                // J-edge processing
                if (y->level <= currentRoot->level) {
                    if (!y->in_phi) {
                        y->in_phi = true;
                        idf.push_back(y->block); // Place Phi function here
                    }
                    if (!y->alpha) {
                        InsertNode(y); // Schedule y to have its frontier explored
                    }
                }
            } else {
                // D-edge processing (x idom y)
                if (!y->visited) {
                    y->visited = true;
                    self(self, y);
                }
            }
        }
    };

    // Pull highest level nodes and evaluate
    while ((currentRoot = GetNode()) != nullptr) {
        currentRoot->visited = true;
        Visit(Visit, currentRoot);
    }

    return idf;
}

// Call this once before placing phis to precompute levels using the DomTree
void SSABuilder::ComputeLevels(Block* entryBlock)
{
    djNodes_.clear();
    maxLevel_ = 0;

    // Setup the entry node
    DJNode* entryNode = GetDJNode(entryBlock);
    entryNode->level = 0;

    // BFS or DFS on Dominator Tree to compute levels
    std::vector<Block*> stack = {entryBlock};
    while (!stack.empty()) {
        Block* current = stack.back();
        stack.pop_back();

        int currentLevel = GetDJNode(current)->level;
        maxLevel_ = std::max(maxLevel_, currentLevel);

        for (Block* child : domTree_.GetChildren(current)) {
            DJNode* childNode = GetDJNode(child);
            childNode->level = currentLevel + 1;
            stack.push_back(child);
        }
    }
}

DJNode* SSABuilder::GetDJNode(Block* block)
{
    auto it = djNodes_.find(block);
    if (it == djNodes_.end()) {
        djNodes_[block] = DJNode{block, 0, false, false, false};
    }
    return &djNodes_[block];
}

void SSABuilder::ResetState()
{
    for (auto& pair : djNodes_) {
        pair.second.visited = false;
        pair.second.alpha = false;
        pair.second.in_phi = false;
    }
}

} // namespace competition

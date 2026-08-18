#ifndef COMPETITION_DOMINATOR_TREE_H
#define COMPETITION_DOMINATOR_TREE_H

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"
#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/Competition/RangeAnalysisSolver/Graph.h"
#include "cangjie/Competition/RangeAnalysisSolver/Solver.h"
#include "cangjie/Competition/Phi.h"

#include <unordered_map>
#include <vector>

namespace Competition {

using namespace Cangjie::CHIR;

/// Computes the dominator tree of a CFG using the
/// Lengauer-Tarjan algorithm.
class DominatorTree {
public:
    explicit DominatorTree(Block* entry, std::vector<Parameter*> &params);

    /// Computes the dominator tree.
    void Compute();

    /// Returns the immediate dominator of a block.
    /// The entry block dominates itself.
    Block* GetImmediateDominator(Block* block) const;

    /// Returns true iff A dominates B.
    bool Dominates(Block* a, Block* b) const;

    /// Returns the children of a node in the dominator tree.
    const std::vector<Block*>& GetChildren(Block* block) const;

    /// Prints the current dominator tree to a .dot file.
    void PrintDominatorTree(const std::string& path, bool alias = false);

    /// Get aliases for identifiers
    std::unordered_map<std::string, std::vector<Block*>> GetAlphaNodes();

    /// Pass along the tree creating constraints on the branches
    void GenerateBranchConstraints() { VisitBlockBranch(entry_); }

    void Renaming();

    void GenerateSSAConstraints();

private:
    struct Node {
        std::vector<Phi> phiFunctions;

        Block* block = nullptr;

        std::size_t dfs = 0;

        std::size_t parent = 0;
        std::size_t semi = 0;
        std::size_t idom = 0;

        std::size_t ancestor = 0;
        std::size_t label = 0;

        std::vector<std::size_t> bucket;

        // Also keep the constraints here
        std::vector<std::shared_ptr<Constraint>> nodeConstraints;

        void pushConstraint(std::shared_ptr<Constraint> constraint) {
            nodeConstraints.push_back(constraint);
        }

    };

    Node* ReverseMapBlockToNode(Block* block);
    void VisitBlockBranch(Block*block);

public:
    std::vector<std::shared_ptr<Constraint>> &GetBlockConstraints(Block *block);
    std::vector<Phi> &GetBlockPhiFunctions(Block *block);
    void AddPhiFunction(Block* block, Phi phiFunction);
    AnalyzedValue GetVariableState(Alias var);
    void CallSolver();
    std::optional<Alias> FindVarBeforeLine(std::string variableName, int lineNumber);
    std::unordered_map<std::string, Alias> idToAlias;

private:
    std::vector<std::string> variables;
    std::unordered_map<std::string, std::vector<Block*>> alphaNodes;
    std::unordered_map<Block*,Node*> blockToNodeMap;

private:
    void DFS(Block* block);

    void Link(std::size_t parent, std::size_t child);
    void Compress(std::size_t v);
    std::size_t Eval(std::size_t v);

private:
    std::string functionName;
    Block* entry_;

    std::vector<Parameter*> params_;

    std::size_t dfsCount_ = 0;

    // Maps CFG block -> DFS number.
    std::unordered_map<Block*, std::size_t> dfsNumber_;

    // Maps DFS number -> CFG block.
    std::vector<Block*> vertex_;

    // Indexed by DFS number.
    std::vector<Node*> nodes_;

    // Immediate dominators.
    std::unordered_map<Block*, Block*> idom_;

    // Dominator tree.
    std::unordered_map<Block*, std::vector<Block*>> children_;

    // Value Range Analysis Abstract State
    AbstractState state;
    bool solverCalled;
public:
    bool IsSolverCalled() { return solverCalled; }
};
} // namespace Competition

#endif // COMPETITION_DOMINATOR_TREE_H
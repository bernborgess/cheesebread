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

typedef std::unordered_map<
    /* fnName */ std::string,
    /* each occurrence */ std::vector<
        /* arguments */ std::vector<Competition::Alias>>>
    ApplyMap;

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

    /// Pass along the tree creating constraints on the branches
    void GenerateBranchConstraints() { VisitBlockBranch(entry_); }

    /// Perform all steps to create Phi functions and rename variables to SSA
    void ConvertToSSA();

    /// Detects calls to Exit() and stores the variables that were returned
    void DetectReturnValues();

private:
    /// Get aliases for identifiers
    void ComputeAlphaNodes();

    void Renaming();

public:
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
    std::vector<Phi>& GetBlockPhiFunctions(Block* block);

private:
    void AddPhiFunction(Block* block, Phi phiFunction);

public:
    std::optional<Alias> FindVarBeforeLine(std::string variableName,
                                           int lineNumber);
    std::unordered_map<std::string, Alias> idToAlias;

private:
    std::vector<std::string> variables;
    // All mutations of `variables` should be here.
    void addVariable(std::string variable);
    std::unordered_map<std::string, std::vector<Block*>> alphaNodes;
    std::unordered_map<Block*,Node*> blockToNodeMap;

private:
    void DFS(Block* block);

    void Link(std::size_t parent, std::size_t child);
    void Compress(std::size_t v);
    std::size_t Eval(std::size_t v);

    std::string functionName;
    Cangjie::CHIR::Type* returnType;
    Block* entry_;
    std::vector<Parameter*> params_;

    // Store all function invocations in here, to create phi later
    ApplyMap arguments_by_functionName;

    // Store aliases of possible return values
    std::vector<Competition::Alias> functionReturnValues;

    // Store invocations to bind their return values later.
    std::unordered_map<std::string, std::vector<Competition::Alias>>
        returnAliases_by_functionName;

   private:
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
public:
    const std::vector<Parameter*>& GetParams() { return params_; };
    const std::string GetFunctionName() { return functionName; };
    const Cangjie::CHIR::Type* GetReturnType() { return returnType; };
    const std::vector<Node*>& GetNodes() { return nodes_; }
    // Map of functions that are invoked and their arguments
    const ApplyMap& GetFnApplyMap() { return arguments_by_functionName; }
    // List of possible aliases returned by this function
    const std::vector<Alias>& GetReturnValues() { return functionReturnValues; }
    // Map of function names to aliases that are defined by their return value
    const std::unordered_map<std::string, std::vector<Competition::Alias>>&
        GetReturnAliasMap() { return returnAliases_by_functionName; };


};
} // namespace Competition

#endif // COMPETITION_DOMINATOR_TREE_H
#include "cangjie/Competition/DominatorTree.h"

#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"
#include <algorithm>
#include <cassert>
#include <fstream>

namespace Competition {

DominatorTree::DominatorTree(Block* entry) : entry_(entry)
{
}

void DominatorTree::Compute()
{
    dfsCount_ = 0;

    dfsNumber_.clear();
    vertex_.clear();
    nodes_.clear();
    idom_.clear();
    children_.clear();

    // Reserve index 0.
    vertex_.push_back(nullptr);
    nodes_.push_back(Node{});

    DFS(entry_);

    //
    // Compute semidominators.
    //
    for (std::size_t i = dfsCount_; i >= 2; --i) {
        Node& w = nodes_[i];

        for (Block* pred : w.block->GetPredecessors()) {
            auto it = dfsNumber_.find(pred);
            if (it == dfsNumber_.end())
                continue;

            std::size_t v = it->second;
            std::size_t u = Eval(v);

            w.semi = std::min(w.semi, nodes_[u].semi);
        }

        nodes_[w.semi].bucket.push_back(i);

        Link(w.parent, i);

        auto& bucket = nodes_[w.parent].bucket;

        for (std::size_t v : bucket) {
            std::size_t u = Eval(v);

            if (nodes_[u].semi < nodes_[v].semi)
                nodes_[v].idom = u;
            else
                nodes_[v].idom = w.parent;
        }

        bucket.clear();
    }

    //
    // Explicitly compute immediate dominators.
    //
    for (std::size_t i = 2; i <= dfsCount_; ++i) {
        if (nodes_[i].idom != nodes_[i].semi)
            nodes_[i].idom = nodes_[nodes_[i].idom].idom;
    }

    nodes_[1].idom = 1;

    //
    // Convert back to Block*.
    //
    for (std::size_t i = 1; i <= dfsCount_; ++i) {
        idom_[nodes_[i].block] = nodes_[nodes_[i].idom].block;
    }

    //
    // Build dominator tree.
    //
    for (std::size_t i = 2; i <= dfsCount_; ++i) {
        Block* child = nodes_[i].block;
        Block* parent = nodes_[nodes_[i].idom].block;

        children_[parent].push_back(child);
    }
}

void DominatorTree::DFS(Block* block)
{
    ++dfsCount_;

    std::size_t n = dfsCount_;

    dfsNumber_[block] = n;

    vertex_.push_back(block);
    nodes_.push_back(Node{});

    Node& node = nodes_[n];

    // Populate the reverse map
    blockToNodeMap[block] = &node;

    node.block = block;
    node.dfs = n;
    node.semi = n;
    node.label = n;

    for (Block* succ : block->GetSuccessors()) {
        if (dfsNumber_.count(succ))
            continue;

        DFS(succ);

        nodes_[dfsNumber_[succ]].parent = n;
    }
}

void DominatorTree::Link(std::size_t parent, std::size_t child)
{
    nodes_[child].ancestor = parent;
}

void DominatorTree::Compress(std::size_t v)
{
    if (nodes_[nodes_[v].ancestor].ancestor == 0)
        return;

    Compress(nodes_[v].ancestor);

    if (nodes_[nodes_[nodes_[v].ancestor].label].semi < nodes_[nodes_[v].label].semi) {
        nodes_[v].label = nodes_[nodes_[v].ancestor].label;
    }

    nodes_[v].ancestor = nodes_[nodes_[v].ancestor].ancestor;
}

std::size_t DominatorTree::Eval(std::size_t v)
{
    if (nodes_[v].ancestor == 0)
        return nodes_[v].label;

    Compress(v);

    return nodes_[v].label;
}

Block* DominatorTree::GetImmediateDominator(Block* block) const
{
    auto it = idom_.find(block);

    if (it == idom_.end())
        return nullptr;

    return it->second;
}

const std::vector<Block*>& DominatorTree::GetChildren(Block* block) const
{
    static const std::vector<Block*> empty;

    auto it = children_.find(block);

    if (it == children_.end())
        return empty;

    return it->second;
}

bool DominatorTree::Dominates(Block* a, Block* b) const
{
    while (true) {
        if (a == b)
            return true;

        auto it = idom_.find(b);

        if (it == idom_.end())
            return false;

        if (it->second == b)
            return false;

        b = it->second;
    }
}

// Same utility from CHIRPrinter
static void ReplaceAll(std::string& str, const std::string& o, const std::string& n)
{
    std::string::size_type pos = 0;
    while ((pos = str.find(o, pos)) != std::string::npos) {
        str.replace(pos, o.length(), n);
        pos += n.length();
    }
}

/// @brief Removes all content of the string after the //
static std::string getUncommented(std::string s)
{
    size_t pos = s.find("//");
    if (pos != std::string::npos) {
        return s.substr(0, pos);
    }
    return s;
}

void DominatorTree::PrintDominatorTree(const std::string& path)
{
    std::fstream fout;
    std::cerr << "DEBUG PRINT DOM TREE!" << path << std::endl;
    fout.open(path, std::ios::out);
    if (!fout.is_open()) {
        std::cerr << "open file: " << path << " failed!" << std::endl;
        return;
    }
    fout << "digraph " << "test " << "{" << std::endl;
    fout << "graph [fontname=\"Courier, monospace\"];" << std::endl;
    fout << "node [fontname=\"Courier, monospace\"];" << std::endl;
    fout << "edge [fontname=\"Courier, monospace\"];" << std::endl;

    // Show block definitions and uses in stderr
    bool DEBUG_DEFS_AND_USES = false;

    for (auto& node : nodes_) {
        // Check that block is valid
        if (node.block == nullptr)
            continue;

        Block* block = node.block;

        // TODO: Obtain proper identifiers here.
        fout << block->GetIdentifierWithoutPrefix();
        fout << " [shape=none, ";
        fout << "label=<<table border='0' cellborder='1' cellspacing='0'>";
        fout << "<tr><td bgcolor='gray' align='center' colspan='1'>";
        fout << "Block" << block->GetIdentifier() << "</td></tr>";

        if (DEBUG_DEFS_AND_USES)
            std::cerr << "Block " << block->GetIdentifier() << " :" << std::endl;

        // Show the CHIR code inside this block!
        for (auto expr : block->GetExpressions()) {
            std::string info = "";

            // ? It's an atribution!
            if (LocalVar* res = expr->GetResult(); res != nullptr) {
                info += res->GetIdentifier() + ": " + res->GetType()->ToString() + " = ";
                if (DEBUG_DEFS_AND_USES)
                    std::cerr << "- DEF " << res->GetIdentifier() + ": " + res->GetType()->ToString() << std::endl;
            }

            if (DEBUG_DEFS_AND_USES)
                // Goes through USES of the block
                for (auto v : expr->GetOperands()) {
                    if (v->IsLiteral())
                        continue;
                    // Filtering out blocks and functions
                    if (v->IsBlock() || v->IsBlockGroup() || v->IsFunc() || v->IsFuncWithBody())
                        continue;

                    std::cerr << "- USE ";

                    // v is either GLOBALVAR, PARAMETER or LOCALVAR
                    if (v->IsGlobalVar())
                        std::cerr << "[GLOBALVAR] ";
                    if (v->IsParameter())
                        std::cerr << "[PARAMETER] ";
                    if (v->IsLocalVar())
                        std::cerr << "[LOCALVAR] ";

                    std::cerr << v->GetIdentifier() + ": " + v->GetType()->ToString() << std::endl;
                }

            // Remove the long comments after the instruction
            info += getUncommented(expr->ToString(0));
            ReplaceAll(info, "&", "&amp;");
            ReplaceAll(info, "<", "&lt;");
            ReplaceAll(info, ">", "&gt;");
            fout << "<tr><td align='left'>" << info << "</td></tr>";
        }
        fout << "</table>>];" << std::endl;

        if (DEBUG_DEFS_AND_USES)
            std::cerr << std::endl;

        // Immediate dominator!
        Block* idom = nodes_[node.idom].block;
        // Prevent root from pointing to itself in the graph
        if (block->GetIdentifierWithoutPrefix() != idom->GetIdentifierWithoutPrefix())
            fout << idom->GetIdentifierWithoutPrefix() << " -> " << block->GetIdentifierWithoutPrefix() << ";"
                 << std::endl;
    }

    fout << "}" << std::endl;
    fout.close();
    if (DEBUG_DEFS_AND_USES)
        std::cerr << std::endl << std::endl;
}

DominatorTree::Node* DominatorTree::reverseMapBlockToNode(Block* block)
{
    if (block == nullptr)
        return nullptr;

    assert(blockToNodeMap.count(block));

    return blockToNodeMap[block];
}

void DominatorTree::GenerateBranchConstraints()
{
    // ? Iterate on the tree, creating Sigmas and constraints
    for (auto& node : nodes_) {
        if (node.block == nullptr)
            continue;

        Block* block = node.block;

        // Only treat blocks that end in a branch
        if (block->GetTerminator()->GetExprKind() != ExprKind::BRANCH)
            continue;

        auto branch = (Branch*)block->GetTerminator();
        auto cond = branch->GetCondition();

        // TODO: All supported pattern matches here

        std::vector<Matching::MatchedConstraints>
            constraints = {// * Match a pattern
                           // Ex: LT(Load(x), Constant(c))
                           Matching::MatchLtVarConst(cond)};

        auto trueNode = reverseMapBlockToNode(branch->GetTrueBlock());
        auto falseNode = reverseMapBlockToNode(branch->GetFalseBlock());

        for (auto& [ifTrue, ifFalse] : constraints) {
            for (auto& constraint : ifTrue)
                trueNode->pushConstraint(constraint);

            for (auto& constraint : ifFalse)
                falseNode->pushConstraint(constraint);
        }
    }
}

} // namespace Competition

#include "cangjie/Competition/DominatorTree.h"
#include "cangjie/Competition/SSABuilder.h"

#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"
#include <algorithm>
#include <cassert>
#include <fstream>

namespace Competition {

DominatorTree::DominatorTree(Block* entry, std::vector<Parameter*> &params)
    : entry_(entry), params_(params)
{
    this->functionName = entry
        ->GetParentBlockGroup()->GetOwnerFunc()->GetSrcCodeIdentifier();
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
    nodes_.push_back(new Node);

    DFS(entry_);

    //
    // Compute semidominators.
    //
    for (std::size_t i = dfsCount_; i >= 2; --i) {
        Node* w = nodes_[i];

        for (Block* pred : w->block->GetPredecessors()) {
            auto it = dfsNumber_.find(pred);
            if (it == dfsNumber_.end())
                continue;

            std::size_t v = it->second;
            std::size_t u = Eval(v);

            w->semi = std::min(w->semi, nodes_[u]->semi);
        }

        nodes_[w->semi]->bucket.push_back(i);

        Link(w->parent, i);

        auto& bucket = nodes_[w->parent]->bucket;

        for (std::size_t v : bucket) {
            std::size_t u = Eval(v);

            if (nodes_[u]->semi < nodes_[v]->semi)
                nodes_[v]->idom = u;
            else
                nodes_[v]->idom = w->parent;
        }

        bucket.clear();
    }

    //
    // Explicitly compute immediate dominators.
    //
    for (std::size_t i = 2; i <= dfsCount_; ++i) {
        if (nodes_[i]->idom != nodes_[i]->semi)
            nodes_[i]->idom = nodes_[nodes_[i]->idom]->idom;
    }

    nodes_[1]->idom = 1;

    //
    // Convert back to Block*.
    //
    for (std::size_t i = 1; i <= dfsCount_; ++i) {
        idom_[nodes_[i]->block] = nodes_[nodes_[i]->idom]->block;
    }

    //
    // Build dominator tree.
    //
    for (std::size_t i = 2; i <= dfsCount_; ++i) {
        Block* child = nodes_[i]->block;
        Block* parent = nodes_[nodes_[i]->idom]->block;

        children_[parent].push_back(child);
    }
}

void DominatorTree::DFS(Block* block)
{
    ++dfsCount_;

    std::size_t n = dfsCount_;

    dfsNumber_[block] = n;

    vertex_.push_back(block);
    nodes_.push_back(new Node);

    Node* node = nodes_[n];

    // Populate the reverse map
    blockToNodeMap[block] = node;

    node->block = block;
    node->dfs = n;
    node->semi = n;
    node->label = n;

    for (Block* succ : block->GetSuccessors()) {
        if (dfsNumber_.count(succ))
            continue;

        DFS(succ);

        nodes_[dfsNumber_[succ]]->parent = n;
    }
}

void DominatorTree::Link(std::size_t parent, std::size_t child)
{
    nodes_[child]->ancestor = parent;
}

void DominatorTree::Compress(std::size_t v)
{
    if (nodes_[nodes_[v]->ancestor]->ancestor == 0)
        return;

    Compress(nodes_[v]->ancestor);

    if (nodes_[nodes_[nodes_[v]->ancestor]->label]->semi < nodes_[nodes_[v]->label]->semi) {
        nodes_[v]->label = nodes_[nodes_[v]->ancestor]->label;
    }

    nodes_[v]->ancestor = nodes_[nodes_[v]->ancestor]->ancestor;
}

std::size_t DominatorTree::Eval(std::size_t v)
{
    if (nodes_[v]->ancestor == 0)
        return nodes_[v]->label;

    Compress(v);

    return nodes_[v]->label;
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

void DominatorTree::ComputeAlphaNodes()
{
    for (auto param : params_) {
        std::string id = param->GetIdentifier();
        std::string funcName = param->GetOwnerFunc()->GetSrcCodeIdentifier();
        std::string aliasDef = param->GetSrcCodeIdentifier() == "" ? id
                             : param->GetSrcCodeIdentifier();
        idToAlias[id] = Alias(funcName, aliasDef);
        idToAlias[id].setCounter(0);
        variables.emplace_back(idToAlias[id].def);
    }

    for (auto node : nodes_) {
        // Check that block is valid
        if (node->block == nullptr)
            continue;

        Block* block = node->block;

        for (auto expr : block->GetExpressions()) {
            auto funcName = expr->GetParentBlockGroup()->GetOwnerFunc()->GetSrcCodeIdentifier();

            if (expr->IsAllocate()) {
                LocalVar* res = expr->GetResult();
                std::string id = res->GetIdentifier();
                std::string aliasDef = res->GetSrcCodeIdentifier() == "" ? id
                                     : res->GetSrcCodeIdentifier();

                idToAlias[id] = Alias(funcName, aliasDef);

                variables.emplace_back(idToAlias[id].def);
                // alphaNodes[res->GetSrcCodeIdentifier()].emplace_back(block);
            }

            // if (expr->IsLoad()) {
            //     auto id = expr->GetResult()->GetIdentifier();
            //     auto opId = expr->GetOperand(0)->GetIdentifier();
            //     if (idToAlias[id].def == "") idToAlias[id] = idToAlias[opId];
            // }

            if (expr->IsStore()) {
                auto id = expr->GetOperand(0)->GetIdentifier();
                auto opId = expr->GetOperand(1)->GetIdentifier();
                // ? Why would this happen?
                if (idToAlias[opId].def == "") continue;
                if (idToAlias[id].def == "")
                    idToAlias[id] = idToAlias[opId];
                alphaNodes[idToAlias[opId].def].emplace_back(block);
            }
        }
    }

    // for (auto [id, alias] : idToAlias) {
    //     if (alias.def != "")
    //         std::cout << id << ": " << alias << "\n";
    // }
}

/// @brief As described in the paper 
/// https://bears.ece.ucsb.edu/class/ece253/papers/cytron91.pdf#page=21 
// Renames all mentions of variables. New variables denoted Vi, where i is an
// integer, are generated for each variable V.
void DominatorTree::Renaming()
{
    for (auto node : nodes_) {
        // Check that block is valid
        if (node->block == nullptr)
            continue;
            
        Block* block = node->block;

        for (auto expr : block->GetExpressions()) {
            if (expr->GetResult() == nullptr) continue;

            auto funcName = expr->GetParentBlockGroup()->GetOwnerFunc()->GetSrcCodeIdentifier();

            std::string id = expr->GetResult()->GetIdentifier();
            if (idToAlias.count(id) == 0) {
                idToAlias[id] = Alias(funcName, id);
                variables.emplace_back(id);
            }
        }
    }

    // We need a loop over all variables only when we initialize two arrays
    // among the following data structures:

    // -S(*) is an array of stacks, one stack for each variable V. The stacks
    // can hold integers. The integer i at the top of S(V) is used to construct
    // the variable Vi that should replace a use of V.
    std::map<std::string, std::stack<int>> variableStack;

    // -C(*) is an array of integers, one for each variable V. The counter value
    // C(V) tells how many assignments to V have been processed.
    std::map<std::string, int> variableCounter;

    for (std::string var : variables) {
        variableCounter[var] = 0;
        variableStack[var] = std::stack<int>();
    }

    auto search = [this, &variableCounter, &variableStack](auto&& self, Node *node) -> void {
        Block *block = node->block;
        if (block == nullptr) return;

        // The visit to a node processes the statements associated with the node
        // in sequential order, starting with any φ-functions that may have been
        // inserted.
        for (auto& phi : node->phiFunctions) {
            // std::cout << "Original: " << phi << "\n";
            std::string varName = phi.getVarDef();
            int counter = variableCounter[varName];
            phi.setVarCounter(counter);
            variableStack[varName].emplace(counter);
            ++variableCounter[varName];
            // std::cout << "Modified: " << phi << "\n";
        }

        // Account for identifiers in the IntersectionConstraints in
        // node->nodeConstraints. We assume that Renaming is called AFTER
        // GenerateBranchConstraints but BEFORE GenerateSSAConstraints,
        // therefore only IntersectionConstraints are inside the
        // node.nodeConstraints.
        for (auto& constraint : node->nodeConstraints) {
            if (auto interc =
                std::dynamic_pointer_cast<IntersectionConstraint>(constraint)) {
                // ? We need to replace the plain variable name stored in
                // these constraints by the updated Aliases::to_string (with counters)

                // Operand
                std::string op = interc->operand;
                int newOpCounter = variableStack[op].top();
                interc->operand = Alias(functionName, op, newOpCounter).to_string();

                // Variable Definition
                std::string var = interc->def;
                int newVarCounter = variableCounter[var];
                interc->def = Alias(functionName, var, newVarCounter).to_string();
                variableStack[var].emplace(newVarCounter);
                ++variableCounter[var];
            }
        }

        // For each expression that's not a phi
        for (auto expr : block->GetExpressions()) {
            if (expr->GetResult() == nullptr) continue;
            // std::cout << "Original: ";
            // expr->Dump();
            
            if (expr->IsConstant()) {
                ;
            } else if (expr->IsApply()) {
                ;
            } else if (expr->IsLoad() ) {
                std::string id = expr->GetResult()->GetIdentifier();
                std::string op = expr->GetOperand(0)->GetIdentifier();
                if (idToAlias[id].def == idToAlias[op].def) {
                    // This is a phi function load
                    for (Phi phiFunction : node->phiFunctions) {
                        if (idToAlias[id].def == phiFunction.getVarDef()) {
                            idToAlias[id].setCounter(phiFunction.getVarCounter());
                        }
                    }
                    continue;
                } else {
                    // We create the constraint here to match the counter of
                    // this use (of 'x') to the current one, not the final
                    // counter of references to this address (idToAlias[%1])
                    std::string var = idToAlias[op].def;
                    int count = variableStack[var].top();

                    // ? Not useful to set loads
                    idToAlias[op].setCounter(count);

                    // Create the constraint here
                    auto type = expr->GetResult()->GetType();
                    if (type->IsInteger()) {
                        auto constraint = std::make_shared<AddConstraint>(
                            Alias(functionName, id, 0).to_string(),
                            Alias(functionName, var, count).to_string(),
                            "\%const_0"
                        );
                        node->pushConstraint(constraint);
                    } else if(type->IsBoolean()) { // Boolean
                        auto constraint = std::make_shared<LogicalAndConstraint>(
                            Alias(functionName, id, 0).to_string(),
                            Alias(functionName, var, count).to_string(),
                            "\%const_true"
                        );
                        node->pushConstraint(constraint);
                    }
                }
            }
            else if (expr->IsAllocate() ||
                     expr->IsDebug() ||
                     expr->IsStore()) {
                continue;
            }
            else {
                for (auto op : expr->GetOperands()) {
                    std::string id = op->GetIdentifier();
                    idToAlias[id].setCounter(variableStack[idToAlias[id].def].top());
                }
            }

            auto var = expr->GetResult();
            std::string varId = var->GetIdentifier();
            std::string varName = idToAlias[varId].def;
            int counter = variableCounter[varName];
            idToAlias[varId].setCounter(counter);
            variableStack[varName].emplace(counter);
            ++variableCounter[varName];

            // if (expr->IsAllocate()) continue;
            // std::cout << "Modified: ";
            // if (expr->IsStore()) {
            //     std::cout << idToAlias[expr->GetOperand(1)->GetIdentifier()]
            //         << " = " << idToAlias[expr->GetOperand(0)->GetIdentifier()];
            // } else {
            //     std::cout << idToAlias[varId] << " = ";
            //     if (expr->IsConstant()) {
            //         std::cout << expr->GetOperand(0)->ToString(0) << "\n";
            //     } else {
            //         for (auto op : expr->GetOperands()) {
            //             if (op != expr->GetOperands().front()) std::cout << " op ";
            //             std::cout << idToAlias[op->GetIdentifier()];
            //         }
            //     }
            // }
            // std::cout << "\n";
        }

        // std::cout << "Computing phi functions of successors\n";
        for (Block *y : block->GetSuccessors()) {
            if (y == nullptr) continue;
            Node *successor = ReverseMapBlockToNode(y);

            int j = 0;
            for (Block *aux : y->GetPredecessors()) {
                if (aux == nullptr) continue;
                // std::cout << "Predecessor " << j << ": " << aux->GetIdentifier() << "\n";
                if (aux == block) break;
                ++j;
            }
            // std::cout << "WhichPred(" << y->GetIdentifier()
            //           << ", " << block->GetIdentifier() << ") = " << j << "\n";
            // std::cout << "Processing successor: " << successor->block->GetIdentifier() << "\n";
            // std::cout << "\t with " << successor->phiFunctions.size() << " phi functions\n";
            for (size_t i = 0; i < successor->phiFunctions.size(); i++) {
                // std::cout << "\t" << successor->phiFunctions[i] << "\n";
                std::string varName = successor->phiFunctions[i].getAliasDefByIdx(j);
                if (variableStack[varName].empty()) {
                    continue;
                    // variableStack[varName].emplace(variableCounter[varName]);
                    // variableCounter[varName]++;
                }
                successor->phiFunctions[i].setAliasCounterByIdx(j, variableStack[varName].top());
            }
        }

        // std::cout << "Visiting children\n";
        for (Block *y : GetChildren(block)) {
            if (y == nullptr) continue;
            // std::cout << "Call search(" << y->GetIdentifier() << ")\n";
            // search(blockToNodeMap[y]);
            self(self, blockToNodeMap[y]);
        }

        // std::cout << "Popping variable stacks\n";
        for (size_t i = 0; i < node->phiFunctions.size(); i++) {
            std::string varName = node->phiFunctions[i].getVarDef();
            variableStack[varName].pop();
        }

        // For each definition of this block, we have to pop it from the stack,
        // since it wont be alive in other branch of the dominator tree
        for (auto expr : block->GetExpressions()) {
            if (expr->GetResult() == nullptr) continue;

            if (expr->IsAllocate() ||
                expr->IsDebug() ||
                expr->IsStore()) continue;

            if (expr->IsLoad()) {
                std::string id = expr->GetResult()->GetIdentifier();
                std::string op = expr->GetOperand(0)->GetIdentifier();
                if (idToAlias[id].def == idToAlias[op].def) continue;
            }

            auto var = expr->GetResult();
            variableStack[idToAlias[var->GetIdentifier()].def].pop();
        }
        // Also for those defined in IntersectionConstraints
        for (auto& constraint : node->nodeConstraints) {
            if (auto interc =
                std::dynamic_pointer_cast<IntersectionConstraint>(constraint)) {
                auto var = Alias::from_string(interc->def).def;
                variableStack[var].pop();
            }
        }
    };

    // Treating the function parameters. 
    // TODO: Interprocedural will create a phi constraint here that binds these
    // to each and every call site of this function
    for (auto param : params_) {
        std::string id = param->GetSrcCodeIdentifier();
        if (id == "") id = param->GetIdentifier(); 
        variableStack[id].emplace(0);
        variableCounter[id] = 1;
    }

    // Begins a top-down traversal of the dominator tree by calling SEARCH at
    // the root node Entry.
    search(search, blockToNodeMap[entry_]);
}

void DominatorTree::PrintDominatorTree(const std::string& path, bool alias)
{
    std::fstream fout;
    fout.open(path, std::ios::out);
    if (!fout.is_open()) {
        std::cerr << "open file: " << path << " failed!" << std::endl;
        return;
    }
    fout << "digraph " << "test " << "{" << std::endl;
    fout << "graph [fontname=\"Courier, monospace\"];" << std::endl;
    fout << "node [fontname=\"Courier, monospace\"];" << std::endl;
    fout << "edge [fontname=\"Courier, monospace\"];" << std::endl;

    for (auto& node : nodes_) {
        // Check that block is valid
        if (node->block == nullptr)
            continue;

        Block* block = node->block;

        fout << block->GetIdentifierWithoutPrefix();
        fout << " [shape=none, ";
        fout << "label=<<table border='0' cellborder='1' cellspacing='0'>";
        fout << "<tr><td bgcolor='gray' align='center' colspan='1'>";
        fout << "Block" << block->GetIdentifier() << "</td></tr>";

        // Show the constraints before the expressions
        for (auto& constraint : node->nodeConstraints) {
            std::ostringstream stream;
            // Casting is needed to invoke the correct operator<<
            if (auto interc = std::dynamic_pointer_cast<IntersectionConstraint>(constraint)) {
                stream << *interc.get();
            } else if (auto phic = std::dynamic_pointer_cast<PhiConstraint>(constraint)) {
                stream << *phic.get();
            }
            if (std::string info = stream.str(); info.length() > 0)
                fout << "<tr><td align='left'>" << info << "</td></tr>";
        }

        // Show the CHIR code inside this block!
        for (auto expr : block->GetExpressions()) {
            std::string info = "";

            // It's an attribution!
            if (LocalVar* res = expr->GetResult(); res != nullptr) {
                auto ident = alias ? idToAlias[res->GetIdentifier()].to_string()
                        : res->GetIdentifier();
                info += ident + ": " + res->GetType()->ToString() + " = ";
            }

            // Remove the long comments after the instruction
            info += getUncommented(expr->ToString(0));
            ReplaceAll(info, "&", "&amp;");
            ReplaceAll(info, "<", "&lt;");
            ReplaceAll(info, ">", "&gt;");
            fout << "<tr><td align='left'>" << info << "</td></tr>";
        }
        fout << "</table>>];" << std::endl;


        // Immediate dominator!
        Block* idom = nodes_[node->idom]->block;
        // Prevent root from pointing to itself in the graph
        if (block->GetIdentifierWithoutPrefix() != idom->GetIdentifierWithoutPrefix())
            fout << idom->GetIdentifierWithoutPrefix() << " -> "
                 << block->GetIdentifierWithoutPrefix() << ";"
                 << std::endl;
    }

    fout << "}" << std::endl;
    fout.close();
}

DominatorTree::Node* DominatorTree::ReverseMapBlockToNode(Block* block)
{
    if (block == nullptr)
        return nullptr;

    assert(blockToNodeMap.count(block));

    return blockToNodeMap[block];
}

std::vector<std::shared_ptr<Constraint>> &DominatorTree::GetBlockConstraints(Block *block) {
    return ReverseMapBlockToNode(block)->nodeConstraints;
}

std::vector<Phi> &DominatorTree::GetBlockPhiFunctions(Block *block) {
    return ReverseMapBlockToNode(block)->phiFunctions;
}

void DominatorTree::AddPhiFunction(Block* block, Phi phiFunction)
{
    if (block == nullptr)
        return;

    Node* node = ReverseMapBlockToNode(block);
    node->phiFunctions.push_back(phiFunction);

    // std::cout << "Added phi function " << phiFunction << " to block " << block->GetIdentifier() << "\n";
}

/// Converting to SSA form = Adding Competition::Alias to each
/// identifier
void DominatorTree::ConvertToSSA() {
    ComputeAlphaNodes();

    std::unordered_map<std::string, std::vector<Block*>> variablePhiNodes;
    for (auto [def, blocks] : alphaNodes) {
        if (blocks.empty()) continue;
        SSABuilder builder(*this);
        variablePhiNodes[def] = builder.PlacePhiNodes(blocks, entry_);
    }

    for (auto [def, phiBlocks] : variablePhiNodes) {
        for (Block* block : phiBlocks) {
            std::string funcName = block->GetParentBlockGroup()
                                       ->GetOwnerFunc()
                                       ->GetSrcCodeIdentifier();
            Phi phiFunction =
                Phi(Alias(funcName, def), block->GetPredecessors().size());
            AddPhiFunction(block, phiFunction);
        }
    }

    Renaming();
}

/// Detects calls to Exit() and stores the variables that were returned
void DominatorTree::DetectReturnValues() {
    // For each Node, check if it's terminator expression is an exit.
    // if so, gather what variables as set as the ret val for the fn
    for (auto& node : nodes_) {
        if (node->block == nullptr) continue;
        Block* block = node->block;

        if (!block->GetTerminator()->IsExit()) continue;

        // This is a return statement of the function. Go back in the
        // expressions to find a store that sets the return value (if exists)
        auto exprs = block->GetExpressions();
        for (auto it = exprs.rbegin(); it != exprs.rend(); ++it) {
            auto expr = *it;
            // Find the last Store in this block
            if (!expr->IsStore()) continue;
            auto store = dynamic_cast<Store*>(expr);

            auto dest = store->GetLocation();
            // that sets on an localVar
            if (!dest->IsLocalVar()) continue;

            auto localVar = dynamic_cast<LocalVar*>(dest);

            // that is a return value
            if (!localVar->IsRetValue()) continue;

            // Store this fact:
            auto returnValueAlias = idToAlias[store->GetValue()->GetIdentifier()];

            functionReturnValues.push_back(returnValueAlias);
        }
    }
}

/// Visit the dominator tree in Pre-Order to guarantee aliases are propagated ok
void DominatorTree::VisitBlockBranch(Block* block)
{
    // Visit it, Only treat blocks that end in a branch
    if (block->GetTerminator()->GetExprKind() == ExprKind::BRANCH) {

        auto branch = dynamic_cast<Branch*>(block->GetTerminator());
        auto cond = branch->GetCondition();
        auto trueNode = ReverseMapBlockToNode(branch->GetTrueBlock());
        auto falseNode = ReverseMapBlockToNode(branch->GetFalseBlock());

        // Matching should consider the aliases
        std::vector<Matching::MatchedConstraints> constraints = {
            // * Match a pattern
            // Ex: LT(Load(x), Constant(c))
            Matching::MatchLessThanConstraints(cond, idToAlias),
            Matching::MatchGreaterThanConstraints(cond, idToAlias),
            Matching::MatchEqualConstraints(cond, idToAlias),
            Matching::MatchNotEqualConstraints(cond, idToAlias),
            Matching::MatchLessEqualConstraints(cond, idToAlias),
            Matching::MatchGreaterEqualConstraints(cond, idToAlias),
        };

        for (auto& [ifTrue, ifFalse] : constraints) {
            for (auto& constraint : ifTrue) {
                auto ptrConstraint = std::make_shared<IntersectionConstraint>(*constraint);
                trueNode->pushConstraint(ptrConstraint);
            }

            for (auto& constraint : ifFalse) {
                auto ptrConstraint = std::make_shared<IntersectionConstraint>(*constraint);
                falseNode->pushConstraint(ptrConstraint);
            }
        }
    }

    // Visit its children
    for (Block* child : children_[block]) {
        VisitBlockBranch(child);
    }
}

void DominatorTree::GenerateSSAConstraints()
{
    // std::cout << "Generating SSA Constraints\n";
    // Set of integer and boolean identifiers
    std::set<std::string> intIdentifiers;
    std::set<std::string> boolIdentifiers;

    // std::cout << "\tGenerating SSA Constraints for parameters\n";

    for (auto param : params_) {
        if (param->GetType()->IsInteger()) {
            Alias paramAlias = idToAlias[param->GetIdentifier()];
            intIdentifiers.emplace(paramAlias.def);
        } else if (param->GetType()->IsBoolean()) {
            Alias paramAlias = idToAlias[param->GetIdentifier()];
            boolIdentifiers.emplace(paramAlias.def);
        }
    }

    // std::cout << "\tGenerating SSA Constraints for nodes\n";

    for (auto& node : nodes_) {
        if (node->block == nullptr)
            continue;

        Block* block = node->block;
        // std::cout << "\t\tBlock: " << block->GetIdentifier() << "\n";

        for (auto phiFunction : node->phiFunctions) {
            if (intIdentifiers.count(phiFunction.getVarDef()) ||
                boolIdentifiers.count(phiFunction.getVarDef())) {
                auto constraint = std::make_shared<PhiConstraint>(
                    phiFunction.getVarString(),
                    phiFunction.getAliasesStrings()
                );
                
                node->pushConstraint(constraint);
            }
        }

        for (auto expr : block->GetExpressions()) {
            if (expr->IsAllocate() ||
                expr->IsDebug() ||
                expr->IsStore()) continue;

            if (expr->IsApply()) {
                auto app = dynamic_cast<Apply*>(expr);

                // ? Here an apply is performed. We have to identify what is the
                // target function 'fnName' and what are the arguments[]
                std::string fnName = app->GetCallee()->GetSrcCodeIdentifier();
                
                std::vector<Competition::Alias> arguments;
                for (auto& arg : app->GetArgs()) {
                    arguments.push_back(idToAlias.at(arg->GetIdentifier()));
                }

                // Register that `fnName` is called with `arguments`
                arguments_by_functionName[fnName].push_back(arguments);

                // Register that `fnName`'s return value is `returnAlias`
                auto returnAlias = idToAlias[app->GetResult()->GetIdentifier()];
                returnAliases_by_functionName[fnName].push_back(returnAlias);

                if (app->GetResultType()->IsInteger()) {
                    intIdentifiers.emplace(
                        idToAlias[app->GetResult()->GetIdentifier()].def);
                } else if (app->GetResultType()->IsBoolean()) {
                    boolIdentifiers.emplace(
                        idToAlias[app->GetResult()->GetIdentifier()].def);
                }
            } else if (expr->IsConstant()) {
                auto cst = dynamic_cast<Constant*>(expr);
                if (cst->IsIntLit()) {
                    Alias def = idToAlias[expr->GetResult()->GetIdentifier()];
                    int val = cst->GetSignedIntLitVal();
                    auto constraint =
                        std::make_shared<InitializationConstraint>(
                            def.to_string(), val);

                    node->pushConstraint(constraint);
                    intIdentifiers.emplace(def.def);
                } else if (cst->IsBoolLit()) {
                    Alias def = idToAlias[expr->GetResult()->GetIdentifier()];
                    bool val = cst->GetBoolLitVal();
                    auto constraint =
                        std::make_shared<InitializationBoolConstraint>(
                            def.to_string(), val);

                    node->pushConstraint(constraint);
                    boolIdentifiers.emplace(def.def);
                }
            } else if (expr->IsUnaryExpr()) {
                auto def = idToAlias[expr->GetResult()->GetIdentifier()];
                auto op = idToAlias[expr->GetOperand(0)->GetIdentifier()];
                if (intIdentifiers.count(op.def)) {
                    if (expr->GetExprKind() == ExprKind::NEG) {
                        auto constraint = std::make_shared<NegConstraint>(
                            def.to_string(), op.to_string());

                        node->pushConstraint(constraint);
                        intIdentifiers.emplace(def.def);
                    } else if (expr->GetExprKind() == ExprKind::BITNOT) {
                        auto constraint =
                            std::make_shared<BitwiseNotConstraint>(
                                def.to_string(), op.to_string());

                        node->pushConstraint(constraint);
                        intIdentifiers.emplace(def.def);
                    }
                } else if (boolIdentifiers.count(op.def)) {
                    if (expr->GetExprKind() == ExprKind::NOT) {
                        auto constraint =
                            std::make_shared<LogicalNotConstraint>(
                                def.to_string(), op.to_string());

                        node->pushConstraint(constraint);
                        boolIdentifiers.emplace(def.def);
                    }
                }
            } else if (expr->IsBinaryExpr()) {
                auto def = idToAlias[expr->GetResult()->GetIdentifier()];
                auto lhs = idToAlias[expr->GetOperand(0)->GetIdentifier()];
                auto rhs = idToAlias[expr->GetOperand(1)->GetIdentifier()];

#define BINARY_CONSTRAINT_PUSH(CONSTRAINT_TYPE, IDENT_TYPE) \
    auto constraint = std::make_shared<CONSTRAINT_TYPE>(    \
        def.to_string(), lhs.to_string(), rhs.to_string()); \
    node->pushConstraint(constraint);                       \
    IDENT_TYPE##Identifiers.emplace(def.def)

                if (intIdentifiers.count(lhs.def) &&
                    intIdentifiers.count(rhs.def)) {

                    switch (expr->GetExprKind()) {
                    case ExprKind::ADD: {
                        BINARY_CONSTRAINT_PUSH(AddConstraint, int);
                        break;
                    }
                    case ExprKind::SUB: {
                        BINARY_CONSTRAINT_PUSH(SubConstraint, int);
                        break;
                    }
                    case ExprKind::MUL: {
                        BINARY_CONSTRAINT_PUSH(MultiplyConstraint, int);
                        break;
                    }
                    case ExprKind::DIV: {
                        BINARY_CONSTRAINT_PUSH(DivConstraint, int);
                        break;
                    }
                    case ExprKind::MOD: {
                        BINARY_CONSTRAINT_PUSH(ModConstraint, int);
                        break;
                    }
                    case ExprKind::LSHIFT: {
                        BINARY_CONSTRAINT_PUSH(ShiftLeftConstraint, int);
                        break;
                    }
                    case ExprKind::RSHIFT: {
                        BINARY_CONSTRAINT_PUSH(ShiftRightConstraint, int);
                        break;
                    }
                    case ExprKind::BITAND: {
                        BINARY_CONSTRAINT_PUSH(BitwiseAndConstraint, int);
                        break;
                    }
                    case ExprKind::BITXOR: {
                        BINARY_CONSTRAINT_PUSH(BitwiseXorConstraint, int);
                        break;
                    }
                    case ExprKind::BITOR: {
                        BINARY_CONSTRAINT_PUSH(BitwiseOrConstraint, int);
                        break;
                    }
                    case ExprKind::EQUAL: {
                        BINARY_CONSTRAINT_PUSH(EqualConstraint, bool);
                        break;
                    }
                    case ExprKind::NOTEQUAL: {
                        BINARY_CONSTRAINT_PUSH(NotEqualConstraint, bool);
                        break;
                    }
                    case ExprKind::LT: {
                        BINARY_CONSTRAINT_PUSH(LessThanConstraint, bool);
                        break;
                    }
                    case ExprKind::GT: {
                        BINARY_CONSTRAINT_PUSH(GreaterThanConstraint, bool);
                        break;
                    }
                    case ExprKind::LE: {
                        BINARY_CONSTRAINT_PUSH(LessEqualConstraint, bool);
                        break;
                    }
                    case ExprKind::GE: {
                        BINARY_CONSTRAINT_PUSH(GreaterEqualConstraint, bool);
                        break;
                    }
                    }

                } else if (boolIdentifiers.count(lhs.def) &&
                           boolIdentifiers.count(rhs.def)) {
                    if (expr->GetExprKind() == ExprKind::AND) {
                        BINARY_CONSTRAINT_PUSH(LogicalAndConstraint, bool);
                    } else if (expr->GetExprKind() == ExprKind::OR) {
                        BINARY_CONSTRAINT_PUSH(LogicalOrConstraint, bool);
                    }
                }
#undef BINARY_CONSTRAINT_PUSH
            } else if (expr->IsLoad()) {
                Alias def = idToAlias[expr->GetResult()->GetIdentifier()];
                Alias op = idToAlias[expr->GetOperand(0)->GetIdentifier()];
                if (intIdentifiers.count(op.def)) {
                    // This is too late to create the constraint with the proper
                    // version of the loaded variable. Better do it inside
                    // renaming.
                    intIdentifiers.emplace(def.def);
                } else if (boolIdentifiers.count(op.def)) {
                    boolIdentifiers.emplace(def.def);
                }
            }
        }
    }
    
}

std::optional<Alias> DominatorTree::FindVarBeforeLine(
    std::string variableName, int lineNumber)
{
    // Get the blocks that contain our lineNumber
    std::queue<Block*> blocks;
    for (auto node : nodes_) {
        if (node->block == nullptr) continue;
        Block* block = node->block;

        size_t start = std::numeric_limits<size_t>::max();
        size_t end = 0;

        for (auto expr : block->GetExpressions()) {
            auto exprLoc = expr->GetDebugLocation().GetBeginPos();
            if (exprLoc.IsZero()) continue;
            size_t exprLine = exprLoc.line;
            start = std::min(exprLine, start);
            end = std::max(exprLine, end);
        }

        if (start <= lineNumber && lineNumber <= end) {
            blocks.push(block);
        }
    }

    // Find alias that matches our variableName, climbing the tree if needed.
    std::optional<Alias> variableAlias = {};
    while(!blocks.empty()) {
        Block* block = blocks.front(); blocks.pop();

        // Look inside phi functions
        for (auto phiFunction : GetBlockPhiFunctions(block)) {
            if (phiFunction.getVarDef() == variableName) {
                variableAlias = phiFunction.getVar();
            }
        }

        // Look inside intersection constraints
        for (auto constraint : GetBlockConstraints(block)) {
            if (auto interc =
                std::dynamic_pointer_cast<IntersectionConstraint>(constraint)) {
                Alias intersectionAlias = Alias::from_string(interc->def);
                if (intersectionAlias.def == variableName) {
                    variableAlias = intersectionAlias;
                }
            }
        }

        // Look in the block's expressions' definitions
        for (auto expr : block->GetExpressions()) {
            auto exprResult = expr->GetResult();
            if (exprResult == nullptr) continue;

            // Only counts if behind or equal lineNumber 
            auto exprLine = expr->GetDebugLocation().GetBeginPos().line;
            if (exprLine > lineNumber) break;

            Alias exprAlias = idToAlias[expr->GetResult()->GetIdentifier()];
            if (exprAlias.def == variableName) {
                variableAlias = exprAlias;
            }
        }

        // If not found yet, consider the dominator
        if(!variableAlias.has_value()) {
            Block* idom = GetImmediateDominator(block);
            if(idom == nullptr || block == idom) continue;
            blocks.push(idom);
        } else {
            // Found the alias. Clean the queue
            while(!blocks.empty()) blocks.pop();
        }
    }

    // Last place to check: Function parameters
    if (!variableAlias.has_value())
        for (Cangjie::CHIR::Parameter* param : params_) {
            if (param->GetSrcCodeIdentifier() == variableName) {
                variableAlias = Alias(functionName, variableName, 0);
            }
        }

    return variableAlias;
}

} // namespace Competition
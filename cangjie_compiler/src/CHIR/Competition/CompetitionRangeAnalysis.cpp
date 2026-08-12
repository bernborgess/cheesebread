#include "cangjie/Competition/CompetitionRangeAnalysis.h"

#include "cangjie/CHIR/Utils/CHIRPrinter.h"
#include "cangjie/Competition/DominatorTree.h"
#include "cangjie/Competition/Phi.h"
#include "cangjie/Competition/SSABuilder.h"
#include "cangjie/Competition/RangeAnalysisSolver/Solver.h"


#include <fstream>
#include <sstream>

namespace Competition {

using namespace Cangjie::CHIR;
using namespace std;

// TODO: Use the alias structure, only casting to_string when in use by the solver.
Alias getAliasFromString(std::string ssaName) {
    auto _pos = ssaName.find_last_of('_');
    return Alias(ssaName.substr(0, _pos), std::stoi(ssaName.substr(_pos+1)));
}

std::string removeIntersectionPrefix(std::string def) {
    while (def.size() >= 2 && (def.substr(0,2) == "\%t" || def.substr(0,2) == "\%f")) {
        def = def.substr(3);
    }
    return def;
}

struct Query {
    string fileName;
    unsigned int lineNumber;
    string variableName;
};

vector<Query> readCompetitionQueries()
{
    // Open the "input.txt" file
    ifstream inputFile;
    inputFile.open("input.txt", ifstream::in);
    if (!inputFile.is_open()) {
        //  No file 'input.txt'
        return {};
    }

    // Read for lineNumber and variableName
    string line;
    vector<Query> queries;
    while (getline(inputFile, line)) {
        // tokenize format [fileName, lineNumber, variableName]
        stringstream ss(line);
        string fileName, variableName;

        getline(ss, fileName, ',');
        fileName.erase(fileName.begin()); // Remove heading [

        unsigned int lineNumber;
        ss >> lineNumber;

        getline(ss, variableName, ','); // Remove comma
        getline(ss, variableName, ',');

        // Remove leading space
        while (*variableName.begin() == ' ') {
            variableName.erase(variableName.begin());
        }

        variableName.erase(variableName.end() - 1); // Remove last ]

        queries.push_back({fileName, lineNumber, variableName});
    }

    inputFile.close();

    return queries;
}

std::vector<Block*> getBlocksByLineNumber(Function* func, unsigned int lineNumber)
{
    auto loc = func->GetDebugLocation();
    std::vector<Block*> blocks;
    if (loc.GetBeginPos().line <= lineNumber && lineNumber <= loc.GetEndPos().line) {
        // 2. Iterate on Blocks to find w.o.c.t. lineNumber
        for (auto block : func->GetBody()->GetBlocks()) {

            size_t start = std::numeric_limits<size_t>::max();
            size_t end = 0;
            for (auto expr : block->GetExpressions()) {
                auto exprLoc = expr->GetDebugLocation().GetBeginPos();
                if (exprLoc.IsZero()) continue;
                size_t exprLine = exprLoc.line;
                start = std::min(exprLine, start);
                end = std::max(exprLine, end);
            }

            // cerr << "Block " << block->GetIdentifier() << " has loc [" << start << ", "
            //         << end << "]" << endl;

            if (start <= lineNumber && lineNumber <= end) {
                blocks.push_back(block);
                // std::cout << "Added block " << block << " with identifier " << block->GetIdentifier() << "\n";
                // Iterate on expressions to find the exact line?
            }
        }
    }
    return blocks;
}

void RangeAnalysis::GatherUsefulBasicBlocks(
        unordered_set<unsigned int>& interestingLineNumbers,
        vector<Function*>& userDefinedFunctions)
{
    for (auto func : userDefinedFunctions) {
        // For each function, iterate on blocks
        for (auto block : func->GetBody()->GetBlocks()) {
            // For each block, iterate on expressions
            for (auto expr : block->GetExpressions()) {
                auto line = expr->GetDebugLocation().GetBeginPos().line;
                // If expression has interestingLine
                if (interestingLineNumbers.count(line)) {
                    // Store in map[interestingLine].push_back(block);
                    blocksByLineNumber[line].push_back(block);
                }
            }
        }
    }
}

// Will recur on the structure and gather all X in Load(X) into the vector
// TODO: Actually, we will define the patterns that matter
void ObtainUsedVars(Value*value, vector<LocalVar*>&vars) {
    // Dead end if not a LocalVar
    if (!value->IsLocalVar())
        return;
    
    auto localVar = (LocalVar*)value;

    auto expr = localVar->GetExpr();

    switch (expr->GetExprMajorKind()) {
        case ExprMajorKind::UNARY_EXPR: {
            auto unaryExpr = (UnaryExpression*)expr;
            ObtainUsedVars(unaryExpr->GetOperand(), vars);
            return;
        }
        case ExprMajorKind::BINARY_EXPR: {
            auto binaryExpr  = (BinaryExpression*)expr;
            ObtainUsedVars(binaryExpr->GetLHSOperand(), vars);
            ObtainUsedVars(binaryExpr->GetRHSOperand(), vars);
            return;
        }
        case ExprMajorKind::MEMORY_EXPR: {
            if(expr->IsLoad()) {
                auto load = (Load*)expr;
                auto variable = load->GetLocation();
                assert(variable->IsLocalVar());
                vars.push_back((LocalVar*)variable);
            }
            return;
        }
        case ExprMajorKind::OTHERS: {
            // Constant, Tuple, Field, Apply, Invoke, Typecast
            if (expr->GetExprKind() == ExprKind::TUPLE) {
                auto tuple = (Tuple*)expr;
                for (auto val : tuple->GetElementValues()) {
                    ObtainUsedVars(val, vars);
                }
            }
            break;
        }
        default: {
            // TERMINATOR, STRUCTURED_CTRL_FLOW_EXPR
            break;
        }
    }
}

void FindBranchesAndTheVariablesTheyUse(vector<Function*>& funcs)
{
    for (auto func : funcs) {
        for (auto block : func->GetBody()->GetBlocks()) {
            // For each block
            if (block->GetTerminator()->GetExprKind() != ExprKind::BRANCH)
                continue;

            // that terminates on a Branch
            auto branch = (Branch*)block->GetTerminator();
            auto cond = branch->GetCondition();

            vector<LocalVar*> vars;
            ObtainUsedVars(cond,vars);

        }
    }
}

void RangeAnalysis::RunOnPackage(Package* package)
{
    // Filter out the builtin cangjie code
    if (package->GetName() == "std.core") return;

    // Reads input file for value range queries
    auto queries = readCompetitionQueries();
    if (queries.size() < 1) return;

    cerr << "@@@@ COMPETITION ANALYSIS @@@@" << endl;

    // Keeps track of Basic Blocks where query[i].lineNumber occurs
    unordered_set<unsigned int> interestingLineNumbers;
    for (auto [fileName, lineNumber, variableName] : queries) {
        interestingLineNumbers.insert(lineNumber);
    }

    // The package contains functions:
    std::set<Function*> requestedFunctions;

    cerr << "Funcs = [ ";
    for (auto func : package->GetGlobalFuncsWithBody()) {
        auto funcFileName = func->GetDebugLocation().GetFileName();
        auto funcStartLine = func->GetDebugLocation().GetBeginPos().line;
        auto funcEndLine = func->GetDebugLocation().GetEndPos().line;
        for (auto [fileName, lineNumber, variableName] : queries) {
            if (funcFileName == fileName) {
                // ? Let's include every function defined in this file (by user)
                // if (funcStartLine <= lineNumber && lineNumber <= funcEndLine) {
                    cerr << func->GetSrcCodeIdentifier() << " ";
                    requestedFunctions.insert(func);
                // }
            }
        }
    }
    std::cerr << "]" << std::endl;

    std::fstream outputFile;
    outputFile.open("output.txt", std::ios::out);
    if(!outputFile.is_open()) {
        std::cerr << "Failed to open output.txt file!"<< std::endl;
        return;
    }

    

    // Create some dominator tree for each function?
    // TODO: Interprocedural
    for (auto func : requestedFunctions) {
        // Our debug
        Block* entry = func->GetEntryBlock();
        std::vector<Parameter*> params = func->GetParams();
        DominatorTree domTree(entry, params);
        // std::cout << "Computing dominator tree\n";
        domTree.Compute();

        // * Intersection constraints use same identifiers ex: x = x ∩ [0,+inf]
        domTree.GenerateBranchConstraints();

        // domTree.GenerateBranchConstraints();
        // if (func->GetSrcCodeIdentifier() == "foo") {
        //     CHIRPrinter::PrintCFG(*func, "foo.dot");
        //     domTree.PrintDominatorTree("domTree.dot");  
        // }

        // std::cout << "Computing alpha nodes\n";
        std::unordered_map<std::string, std::vector<Block*>>
            alphaNodes = domTree.GetAlphaNodes();
        
        // std::cout << "Computing phi nodes\n";
        std::unordered_map<std::string, std::vector<Block*>> variablePhiNodes;
        for (auto [def, blocks] : alphaNodes) {
            if (blocks.empty()) continue;
            // std::cout << "Blocks with definitions of " << def << ": [";
            // for (Block *block : blocks) {
            //     if (block != *blocks.begin()) std::cout << ", ";
            //     std::cout << block->GetIdentifier();
            // }
            // std::cout << "]\n";
            SSABuilder builder(domTree);
            variablePhiNodes[def] = builder.PlacePhiNodes(blocks, func->GetBody()->GetEntryBlock());
        }

        // We want to rename everything

        // std::cout << "Constructing phi functions\n";
        for (auto [def, phiBlocks] : variablePhiNodes) {
            for (Block *block : phiBlocks) {
                std::string funcName = block->GetParentBlockGroup()->GetOwnerFunc()->GetSrcCodeIdentifier();
                Phi phiFunction = Phi(Alias(funcName + ":" + def), block->GetPredecessors().size());
                domTree.AddPhiFunction(block, phiFunction);
            }
        }

        // for (auto [def, phiBlocks] : variablePhiNodes) {
        //     if (phiBlocks.empty()) continue;
        //     std::cout << "Phi blocks of " << def << ": [";
        //     for (Block *block : phiBlocks) {
        //         if (block != *phiBlocks.begin()) std::cout << ", ";
        //         std::cout << block->GetIdentifier();
        //     }
        //     std::cout << "]\n";
        // }

        // std::cout << "Applying renaming\n";

        // if (func->GetSrcCodeIdentifier() == "foo") {
            
        domTree.Renaming();
        // Produce the graph after renaming alias.
        domTree.PrintDominatorTree("domTreeSSA.dot", true);  
        // TODO: Add intersection constriants on branches
        // * Careful to use the right SSA names in the constraint.
        
        domTree.GenerateSSAConstraints();

        domTree.CallSolver();
        
        
        
        
        auto funcFileName = func->GetDebugLocation().GetFileName();
        auto funcStartLine = func->GetDebugLocation().GetBeginPos().line;
        auto funcEndLine = func->GetDebugLocation().GetEndPos().line;
        // Use the solver results to output the analsys
        for (auto [fileName, lineNumber, variableName] : queries) {

            if (funcFileName != fileName) continue;
            if (funcStartLine > lineNumber || funcEndLine < lineNumber) continue;

            std::cerr << "Find the range of variable " << variableName
                    << " at line " << lineNumber << " of file " << fileName
                    << std::endl;
                
            std::queue<Block*> blocks;
            for (auto block : getBlocksByLineNumber(func,lineNumber)) {
                blocks.push(block);
            }

            Alias variableAlias = Alias("\%empty");
            while (!blocks.empty()) {
                Block* block = blocks.front();
                blocks.pop();
                // std::cout << "Searching for definition of " << variableName << " on block " << block->GetIdentifier() << "\n";
                
                // std::cout << "Searching phi functions\n";
                for (auto phiFunction : domTree.GetBlockPhiFunctions(block)) {
                    if (phiFunction.getVarDef() == variableName) {
                        variableAlias = phiFunction.getVar();
                    }
                }
                
                // std::cout << "Searching intersection constraints\n";
                for (auto constraint : domTree.GetBlockConstraints(block)) {
                    if (auto interc = std::dynamic_pointer_cast<IntersectionConstraint>(constraint)) {
                        Alias intersectionAlias = getAliasFromString(interc->def);
                        if (removeIntersectionPrefix(intersectionAlias.def) == variableName) {
                            variableAlias = intersectionAlias;
                        }
                    }
                }
                
                // std::cout << "Searching expressions\n";
                for (auto expr : block->GetExpressions()) {
                    auto exprResult = expr->GetResult();
                    if (exprResult == nullptr) continue;
                    auto exprLine = expr->GetDebugLocation().GetBeginPos().line;
                    if (exprLine > lineNumber) break;
                    Alias exprAlias = domTree.idToAlias[expr->GetResult()->GetIdentifier()];
                    if (exprAlias.def == variableName) {
                        variableAlias = exprAlias;
                    }
                }
                
                if (variableAlias.def == "\%empty") {
                    Block* idom = domTree.GetImmediateDominator(block);
                    if (idom == nullptr || block == idom) continue;
                    blocks.push(idom);
                } else {
                    while (!blocks.empty()) {
                        blocks.pop();
                    }
                }
            }
            
            AnalyzedValue variableValue = domTree.GetVariableState(variableAlias);
            
            // ? For now just using the default range => no info
            if (std::holds_alternative<BV>(variableValue)) {
                auto boolVal = std::get<BV>(variableValue);
                outputFile << boolVal << std::endl;
            } else {
                auto intVal = std::get<IV>(variableValue);
                outputFile << intVal << std::endl;
            }
        }
    // }
        
    }

    outputFile.close();
    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
    return;
}

} // namespace Competition

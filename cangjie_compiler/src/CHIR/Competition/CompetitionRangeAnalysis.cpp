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

Block* getBlockByLineNumber(vector<Function*>& funcs, unsigned int lineNumber)
{
    // 1. Iterate on our functions to find which one contains the lineNumber
    for (auto& func : funcs) {
        auto loc = func->GetDebugLocation();
        if (loc.GetBeginPos().line <= lineNumber && lineNumber <= loc.GetEndPos().line) {
            // 2. Iterate on Blocks to find w.o.c.t. lineNumber
            for (auto& block : func->GetBody()->GetBlocks()) {
                auto blockLoc = block->GetDebugLocation();

                cerr << "Block " << block->GetIdentifier() << " has loc [" << blockLoc.GetBeginPos().line << ", "
                     << blockLoc.GetEndPos().line << "]" << endl;

                if (blockLoc.GetBeginPos().line <= lineNumber && lineNumber <= blockLoc.GetEndPos().line) {
                    return block;
                    // Iterate on expressions to find the exact line?
                }
            }
        }
    }
    return nullptr;
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
    vector<Function*> userDefinedFunctions;

    cerr << "Funcs = [ ";
    for (auto func : package->GetGlobalFuncsWithBody()) {
        if (func->GetPackageName() != "std.core") {
            // TODO: Filter user-defined functions only (still reporting $mainInvoke, etc.)
            cerr << func->GetSrcCodeIdentifier() << " ";
            userDefinedFunctions.push_back(func);
        }
    }
    std::cerr << "]" << std::endl;

    // Create some dominator tree for each function?
    // TODO: Interprocedural
    for (auto func : userDefinedFunctions) {
        // Our debug
        Block* entry = func->GetEntryBlock();
        DominatorTree domTree(entry);
        // std::cout << "Computing dominator tree\n";
        domTree.Compute();
        // domTree.GenerateBranchConstraints();
        if (func->GetSrcCodeIdentifier() == "foo") {
            CHIRPrinter::PrintCFG(*func, "foo.dot");
            domTree.PrintDominatorTree("domTree.dot");  
        }

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
                Phi phiFunction = Phi(Alias(def), alphaNodes[def].size());
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

        if (func->GetSrcCodeIdentifier() == "foo") {
            domTree.Renaming();

            domTree.GenerateSSAConstraints();
             
            // Produce the graph after renaming alias.
            domTree.PrintDominatorTree("domTreeSSA.dot", true);  
        }



        // TODO: Add intersection constriants on branches
        // * Careful to use the right SSA names in the constraint.
        // domTree.GenerateBranchConstraints();
    }

    // 1. Initialize a clean abstract state table
    AbstractState state;
    // TODO: After SSA;   
    Solver solver(state);

    std::fstream outputFile;
    outputFile.open("output.txt", std::ios::out);
    if(!outputFile.is_open()) {
        std::cerr << "Failed to open output.txt file!"<< std::endl;
        return;
    }

    // Use the solver results to output the analsys
    for (auto [fileName, lineNumber, variableName] : queries) {
        std::cerr << "Find the range of variable " << variableName
                  << " at line " << lineNumber << " of file " << fileName
                  << std::endl;

        for (auto block : blocksByLineNumber[lineNumber]) {
            // TODO: Gather abstract value from this block, about the
            // variableName
            block->GetIdentifier();
        }

        // ? For now just using the default range => no info
        if (std::holds_alternative<BV>(state[variableName])) {
            auto boolVal = std::get<BV>(state[variableName]);
            outputFile << boolVal << std::endl;
        } else {
            auto intVal = std::get<IV>(state[variableName]);
            outputFile << intVal << std::endl;
        }
    }

    outputFile.close();
    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
    return;
}

} // namespace Competition

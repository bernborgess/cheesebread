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

struct Query {
    std::string fileName;
    unsigned int lineNumber;
    std::string variableName;
};

std::vector<Query> readCompetitionQueries()
{
    // Open the "input.txt" file
    std::ifstream inputFile;
    inputFile.open("input.txt", std::ifstream::in);
    if (!inputFile.is_open()) {
        //  No file 'input.txt'
        return {};
    }

    // Read for lineNumber and variableName
    std::string line;
    std::vector<Query> queries;
    while (getline(inputFile, line)) {
        // tokenize format [fileName, lineNumber, variableName]
        std::stringstream ss(line);
        std::string fileName, variableName;

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

void RangeAnalysis::RunOnPackage(Package* package)
{
    // Filter out the builtin cangjie code
    if (package->GetName() == "std.core") return;

    // Reads input file for value range queries
    auto queries = readCompetitionQueries();
    if (queries.size() < 1) return;

    std::cerr << "@@@@ COMPETITION ANALYSIS @@@@" << std::endl;

    // Keeps track of Basic Blocks where query[i].lineNumber occurs
    std::unordered_set<unsigned int> interestingLineNumbers;
    for (auto [fileName, lineNumber, variableName] : queries) {
        interestingLineNumbers.insert(lineNumber);
    }

    // The package contains functions:
    std::set<Function*> requestedFunctions;

    for (auto func : package->GetGlobalFuncsWithBody()) {
        auto funcFileName = func->GetDebugLocation().GetFileName();
        for (auto [fileName, lineNumber, variableName] : queries) {
            if (funcFileName == fileName) {
                requestedFunctions.insert(func);
            }
        }
    }

    std::fstream outputFile;
    outputFile.open("output.txt", std::ios::out);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output.txt file!" << std::endl;
        return;
    }

    std::vector<DominatorTree*> allDomTrees;
    std::vector<std::optional<DominatorTree*>>
        queryToDomTree(queries.size());

    // Compute dominator tree for each function, insert the intraprocedural
    // constraints
    for (auto func : requestedFunctions) {
        Block* entry = func->GetEntryBlock();
        std::vector<Parameter*> params = func->GetParams();

        // Create with new to store references by query, later needed to gather
        // correct identifiers
        auto domTree = new DominatorTree(entry, params);
        allDomTrees.push_back(domTree);

        domTree->Compute();

        // Produce graph before renaming (func foo only, for debug)
        if (func->GetSrcCodeIdentifier() == "foo")
            domTree->PrintDominatorTree("domTreeBefore.dot");

        // Intersection constraints use same identifiers ex: x = x ∩ [0,+inf]
        domTree->GenerateBranchConstraints();

        /// Converting to SSA form = Adding Competition::Alias to each
        /// identifier
        std::unordered_map<std::string, std::vector<Block*>> alphaNodes =
            domTree->GetAlphaNodes();

        std::unordered_map<std::string, std::vector<Block*>> variablePhiNodes;
        for (auto [def, blocks] : alphaNodes) {
            if (blocks.empty()) continue;
            SSABuilder builder(*domTree);
            variablePhiNodes[def] =
                builder.PlacePhiNodes(blocks, func->GetBody()->GetEntryBlock());
        }

        for (auto [def, phiBlocks] : variablePhiNodes) {
            for (Block* block : phiBlocks) {
                std::string funcName = block->GetParentBlockGroup()
                                           ->GetOwnerFunc()
                                           ->GetSrcCodeIdentifier();
                Phi phiFunction = Phi(Alias(funcName, def),
                                      block->GetPredecessors().size());
                domTree->AddPhiFunction(block, phiFunction);
            }
        }

        domTree->Renaming();
        /// END Converting to SSA form

        domTree->GenerateSSAConstraints();

        // Produce the graph after renaming alias (func foo only, for debug).
        if (func->GetSrcCodeIdentifier() == "foo")
            domTree->PrintDominatorTree("domTreeSSA.dot", true);

        auto funcFileName = func->GetDebugLocation().GetFileName();
        auto funcStartLine = func->GetDebugLocation().GetBeginPos().line;
        auto funcEndLine = func->GetDebugLocation().GetEndPos().line;

        // Store reference to domTree of each query
        for (int i = 0; i < queries.size(); i++) {
            auto& [fileName, lineNumber, variableName] = queries[i];
            if (funcFileName != fileName) continue;
            if (funcStartLine > lineNumber || funcEndLine < lineNumber)
                continue;

            // This query is solved on the dominator tree.
            // We still need to bind the interprocedural calls, only after that
            // we call the solver.
            queryToDomTree[i] = domTree;
        }

    }

    // TODO: Interprocedural
    // * For each function Apply, bind the identifiers of the source function
    // to the target function parameters phi fn.
    // * For each return in target function, add the abstract value to the ret
    // value to the source phi fn.

    // General constraints for 0 and true
    auto cst_0 = std::make_shared<InitializationConstraint>("\%const_0", 0);
    auto cst_true =
        std::make_shared<InitializationBoolConstraint>("\%const_true", true);
    constraintGraph.addConstraint(cst_0);
    constraintGraph.addConstraint(cst_true);

    std::cerr << "All the constraints: " << std::endl;
    for (auto& domTree : allDomTrees) {
        // ! Temporary code before we bind the parameters
        for (auto& param : domTree->GetParams()) {
            if (param->GetType()->IsInteger()) {
                Alias paramAlias = Alias(domTree->GetFunctionName(),
                                         param->GetSrcCodeIdentifier(), 0);
                auto constraint = std::make_shared<InitializationIntegerTop>(
                    paramAlias.to_string());
                constraintGraph.addConstraint(constraint);
                std::cerr << *constraint << std::endl;
            } else if (param->GetType()->IsBoolean()) {
                Alias paramAlias = Alias(domTree->GetFunctionName(),
                                         param->GetSrcCodeIdentifier(), 0);
                auto constraint = std::make_shared<InitializationBoolTop>(
                    paramAlias.to_string());
                constraintGraph.addConstraint(constraint);
                std::cerr << *constraint << std::endl;
            }
        }

        // TODO: Bind params and arguments with phi function

        // Include the constraints in the Nodes of the domTree (by function)
        for (auto& node : domTree->GetNodes()) {
            for (auto& constraint : node->nodeConstraints) {
                constraintGraph.addConstraint(constraint);
                std::cerr << constraint << std::endl;
            }
        }
    }
    std::cerr << "END All the constraints." << std::endl;

    // Call the solver
    auto sccs = constraintGraph.getTopologicalSCCs();
    Solver solver(solverState);
    solver.solve(sccs);

    // Use the solver results to output the analsys
    for (int i = 0; i < queries.size(); i++) {
        if(!queryToDomTree[i].has_value()) {
            std::cerr << "No domTree was found for query!" << std::endl;
            // TODO: Output bottom range here.
            continue;
        }

        DominatorTree* domTree = queryToDomTree[i].value();

        auto& [fileName, lineNumber, variableName] = queries[i];
        std::cerr << "Find the range of variable " << variableName
                  << " at line " << lineNumber
                  << " of file " << fileName << std::endl;

        auto maybeVariableAlias = domTree->FindVarBeforeLine(variableName, lineNumber);
        if (!maybeVariableAlias.has_value()) {
            std::cerr << "No alias for variable \"" << variableName
                      << "\" was found for query before line " << lineNumber
                      << "!" << std::endl;
            // TODO: Output bottom range here.
            continue;
        }

        Alias variableAlias = maybeVariableAlias.value();
        std::cerr << "You want " << variableAlias.to_string() << ", ";
        AnalyzedValue variableValue = solverState[variableAlias.to_string()];

        // ? For now just using the default range => no info
        if (std::holds_alternative<BV>(variableValue)) {
            auto boolVal = std::get<BV>(variableValue);
            outputFile << boolVal << std::endl;
            std::cerr << "Boolean range: " << boolVal << std::endl;
        } else {
            auto intVal = std::get<IV>(variableValue);
            outputFile << intVal << std::endl;
            std::cerr << "Integer range: " << intVal << std::endl;
        }
    }

    // Free created domTrees
    for (auto ptr : allDomTrees) {
        delete ptr;
    }

    outputFile.close();
    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
    return;
}

} // namespace Competition

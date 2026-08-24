#include "cangjie/Competition/CompetitionRangeAnalysis.h"

#include "cangjie/CHIR/Utils/CHIRPrinter.h"
#include "cangjie/Competition/DominatorTree.h"
#include "cangjie/Competition/Phi.h"
#include "cangjie/Competition/SSABuilder.h"
#include "cangjie/Competition/RangeAnalysisSolver/Solver.h"
#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"


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

    std::unordered_map<std::string, DominatorTree*> domTree_by_fnName;
    std::vector<std::optional<DominatorTree*>> queryToDomTree(queries.size());

    // Compute dominator tree for each function, insert the intraprocedural
    // constraints
    for (auto func : requestedFunctions) {
        Block* entry = func->GetEntryBlock();
        std::vector<Parameter*> params = func->GetParams();

        // Create with new to store references by query, later needed to gather
        // correct identifiers
        auto domTree = new DominatorTree(entry, params);
        domTree_by_fnName[func->GetSrcCodeIdentifier()] = domTree;

        domTree->Compute();

        // Produce graph before renaming (func foo only, for debug)
        if (func->GetSrcCodeIdentifier() == "foo")
            domTree->PrintDominatorTree("domTreeBefore.dot");
        if (func->GetSrcCodeIdentifier() == "main")
            domTree->PrintDominatorTree("MaindomTree.dot");

        // Intersection constraints use same identifiers ex: x = x ∩ [0,+inf]
        domTree->GenerateBranchConstraints();

        domTree->ConvertToSSA();

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

    ApplyMap argumentsByFnName;
    for (auto& [_, domTree] : domTree_by_fnName) {
        const auto applyMap = domTree->GetFnApplyMap();
        argumentsByFnName.insert(applyMap.begin(), applyMap.end());
    }

    std::cerr << "All the constraints: " << std::endl;
    for (auto& [callee, invocations] : argumentsByFnName) {
        if (invocations.size() == 0 || domTree_by_fnName.count(callee) == 0) {
            continue;
        }

        // Insert these as arguments to a phi function at the start of callee
        auto domTree = domTree_by_fnName[callee];
        auto params = domTree->GetParams();
        for (int i = 0; i < params.size(); i++) {
            std::vector<std::string> ops;

            for (auto& args : invocations) {
                // All invocation MUST have the same number of parameters
                assert(args.size() == params.size());

                ops.push_back(args[i].to_string());
            }

            auto paramName = params[i]->GetSrcCodeIdentifier();
            auto alias = Alias(callee, paramName, 0);
            auto phi = std::make_shared<PhiConstraint>(alias.to_string(), ops);
            constraintGraph.addConstraint(phi);

            std::cerr << *phi << std::endl;
        }
    }

    // TODO: Gather all return statements inside the function to map back to a
    // constraint in the caller's return variable
    // * For each return in target function, add the abstract value to the ret
    // value to the source phi fn.

    // General constraints for 0 and true
    auto cst_0 = std::make_shared<InitializationConstraint>("\%const_0", 0);
    auto cst_true =
        std::make_shared<InitializationBoolConstraint>("\%const_true", true);
    constraintGraph.addConstraint(cst_0);
    constraintGraph.addConstraint(cst_true);

    for (auto& [_, domTree] : domTree_by_fnName) {
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
    for (auto [_, ptr] : domTree_by_fnName) {
        delete ptr;
    }

    outputFile.close();
    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
    return;
}

} // namespace Competition

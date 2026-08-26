#include "cangjie/Competition/CompetitionRangeAnalysis.h"

#include "cangjie/CHIR/Utils/CHIRPrinter.h"
#include "cangjie/Competition/Phi.h"
#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"
#include "cangjie/Competition/RangeAnalysisSolver/Solver.h"
#include "cangjie/Competition/SSABuilder.h"

#include <fstream>
#include <sstream>

namespace Competition {

using namespace Cangjie::CHIR;

void RangeAnalysis::ReadCompetitionQueries()
{
    // Open the "input.txt" file
    std::ifstream inputFile;
    inputFile.open("input.txt", std::ifstream::in);
    if (!inputFile.is_open()) { //  No file 'input.txt'
        return;
    }

    // Read for lineNumber and variableName
    std::string line;
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

        queries.push_back({ fileName, lineNumber, variableName });
    }

    inputFile.close();
}

void RangeAnalysis::GatherRequestedFunctions(Cangjie::CHIR::Package* package)
{
    for (auto func : package->GetGlobalFuncsWithBody()) {
        auto funcFileName = func->GetDebugLocation().GetFileName();
        for (auto [fileName, lineNumber, variableName] : queries) {
            if (funcFileName == fileName) {
                requestedFunctions.insert(func);
            }
        }
    }
}

void RangeAnalysis::BuildDomTreeWithConstraints(Cangjie::CHIR::Function* func)
{
    Block* entry = func->GetEntryBlock();
    std::vector<Parameter*> params = func->GetParams();

    // Create with new to store references by query, later needed to gather
    // correct identifiers
    auto funcName = func->GetSrcCodeIdentifier();
    auto domTree = new DominatorTree(entry, params);
    domTree_by_fnName[funcName] = domTree;

    domTree->Compute();

    // Produce graph before renaming
    domTree->PrintDominatorTree(funcName + "-domTree.dot");

    // Intersection constraints use same identifiers ex: x = x ∩ [0,+inf]
    domTree->GenerateBranchConstraints();

    domTree->ConvertToSSA();

    domTree->GenerateSSAConstraints();

    // Go after the return values, these will be used to assign as the value
    // of an Apply from other (or same) function
    domTree->DetectReturnValues();

    // Produce the graph after renaming alias
    domTree->PrintDominatorTree(funcName + "-ssa.dot", true);

    auto funcFileName = func->GetDebugLocation().GetFileName();
    auto funcStartLine = func->GetDebugLocation().GetBeginPos().line;
    auto funcEndLine = func->GetDebugLocation().GetEndPos().line;

    // Store reference to domTree of each query
    for (int i = 0; i < queries.size(); i++) {
        auto& [fileName, lineNumber, variableName] = queries[i];
        if (funcFileName != fileName)
            continue;
        if (funcStartLine > lineNumber || funcEndLine < lineNumber)
            continue;

        // This query is solved on the dominator tree.
        // We still need to bind the interprocedural calls, only after that
        // we call the solver.
        queryToDomTree[i] = domTree;
    }

    // Inserting the created constraints
    for (auto& node : domTree->GetNodes()) {
        for (auto& constraint : node->nodeConstraints) {
            constraintGraph.addConstraint(constraint);
            std::cerr << constraint << std::endl;
        }
    }
}

// For each function Apply, bind the identifiers of the source function
// to the target function parameters with phi functions.
void RangeAnalysis::BindArgumentsToParamsWithPhiConstraint()
{
    ApplyMap argumentsByFnName;
    for (auto& [_, domTree] : domTree_by_fnName) {
        const auto applyMap = domTree->GetFnApplyMap();
        argumentsByFnName.insert(applyMap.begin(), applyMap.end());
    }

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
}

// For each return value in target function, bind it to the call result with a
// phi function
void RangeAnalysis::BindReturnValuesToCallResultsWithPhiConstraint()
{
    for (auto& [fnName, domTree] : domTree_by_fnName) {
        // Include the constraints in the Nodes of the domTree (by function)
        for (auto& node : domTree->GetNodes()) {
            for (auto& constraint : node->nodeConstraints) {
                constraintGraph.addConstraint(constraint);
                std::cerr << constraint << std::endl;
            }
        }

        // Debugging the returnAliases
        for (auto [callee, vals] : domTree->GetReturnAliasMap()) {
            if (!domTree_by_fnName.count(callee))
                continue;

            std::vector<std::string> ops;
            for (auto rv : domTree_by_fnName[callee]->GetReturnValues()) {
                ops.push_back(rv.to_string());
            }

            // !DEBUG: ops aren't all the same type (BV, IV)
            // Check that getInt return var ACTUALLY becomes an IV.

            for (auto& val : vals) {
                auto phi = std::make_shared<PhiConstraint>(val.to_string(), ops);
                constraintGraph.addConstraint(phi);
                std::cerr << *phi << std::endl;
            }
        }
    }
}

void RangeAnalysis::CreateHelperConstraints()
{
    auto cst_0 = std::make_shared<InitializationConstraint>("\%const_0", 0);
    auto cst_true = std::make_shared<InitializationBoolConstraint>("\%const_true", true);
    constraintGraph.addConstraint(cst_0);
    constraintGraph.addConstraint(cst_true);
}

void RangeAnalysis::OutputAnalysisToFile()
{
    std::fstream outputFile;
    outputFile.open("output.txt", std::ios::out);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output.txt file!" << std::endl;
        return;
    }

    for (int i = 0; i < queries.size(); i++) {
        if (!queryToDomTree[i].has_value()) {
            std::cerr << "No domTree was found for query!" << std::endl;
            // TODO: Output bottom range here.
            continue;
        }

        DominatorTree* domTree = queryToDomTree[i].value();

        auto& [fileName, lineNumber, variableName] = queries[i];
        std::cerr << "Find the range of variable " << variableName << " at line "
                  << lineNumber << " of file " << fileName << std::endl;

        auto maybeVariableAlias = domTree->FindVarBeforeLine(variableName, lineNumber);
        if (!maybeVariableAlias.has_value()) {
            std::cerr << "No alias for variable \"" << variableName
                      << "\" was found for query before line " << lineNumber << "!"
                      << std::endl;
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
    outputFile.close();
}

void RangeAnalysis::RunOnPackage(Package* package)
{
    // Filter out the builtin cangjie code
    if (package->GetName() == "std.core")
        return;

    // Reads input file for value range queries
    ReadCompetitionQueries();
    if (queries.size() < 1)
        return;

    queryToDomTree.resize(queries.size());

    std::cerr << "@@@@ COMPETITION ANALYSIS @@@@" << std::endl;

    GatherRequestedFunctions(package);

    // Compute dominator tree for each function, insert the intraprocedural
    // constraints
    for (auto func : requestedFunctions)
        BuildDomTreeWithConstraints(func);

    // Interprocedural
    BindArgumentsToParamsWithPhiConstraint();
    BindReturnValuesToCallResultsWithPhiConstraint();

    // General constraints for 0 and true
    CreateHelperConstraints();

    // Call the solver
    auto sccs = constraintGraph.getTopologicalSCCs();
    Solver solver(solverState);
    solver.solve(sccs);

    // Use the solver results to output the analysis
    OutputAnalysisToFile();

    // Free created domTrees
    for (auto [_, ptr] : domTree_by_fnName) {
        delete ptr;
    }

    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
    return;
}

} // namespace Competition

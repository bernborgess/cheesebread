#include "cangjie/Competition/CompetitionRangeAnalysis.h"

#include <fstream>
#include <sstream>

#include "cangjie/CHIR/Utils/CHIRPrinter.h"
#include "cangjie/Competition/Phi.h"
#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"
#include "cangjie/Competition/RangeAnalysisSolver/Solver.h"
#include "cangjie/Competition/SSABuilder.h"

namespace Competition {

using namespace Cangjie::CHIR;

// Define this to show all constraints added to the constraint graph
// #define DEBUG_SHOW_INSERTED_CONSTRAINTS
// Define this to log the queries to stderr
// #define DEBUG_PRINT_QUERIES
// Define this to evaluate Metric 1: Range Reduction

void RangeAnalysis::ReadCompetitionQueries() {
    // Open the "input.txt" file
    std::ifstream inputFile;
    inputFile.open("input.txt", std::ifstream::in);
    if (!inputFile.is_open()) {  //  No file 'input.txt'
        return;
    }

    // Read for lineNumber and variableName
    std::string line;
    while (getline(inputFile, line)) {
        // tokenize format [fileName, lineNumber, variableName]
        std::stringstream ss(line);
        std::string fileName, variableName;

        getline(ss, fileName, ',');
        fileName.erase(fileName.begin());  // Remove heading [

        unsigned int lineNumber;
        ss >> lineNumber;

        getline(ss, variableName, ',');  // Remove comma
        getline(ss, variableName, ',');

        // Remove leading space
        while (*variableName.begin() == ' ') {
            variableName.erase(variableName.begin());
        }

        variableName.erase(variableName.end() - 1);  // Remove last ]

        queries.push_back({fileName, lineNumber, variableName});
    }

    inputFile.close();
}

void RangeAnalysis::GatherRequestedFunctions(Cangjie::CHIR::Package* package) {
    for (auto func : package->GetGlobalFuncsWithBody()) {
        auto funcFileName = func->GetDebugLocation().GetFileName();
        for (auto [fileName, lineNumber, variableName] : queries) {
            if (funcFileName == fileName) {
                auto funcSrcId = func->GetSrcCodeIdentifier();
                if (funcSrcId.find('$') != std::string::npos)
                    continue;  // Internal function
                requestedFunctions.insert(func);
            }
        }
    }
}

void RangeAnalysis::BuildDomTreeWithConstraints(Cangjie::CHIR::Function* func) {
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
        if (funcFileName != fileName) continue;
        if (funcStartLine > lineNumber || funcEndLine < lineNumber) continue;

        // This query is solved on the dominator tree.
        // We still need to bind the interprocedural calls, only after that
        // we call the solver.
        queryToDomTree[i] = domTree;
    }

    // Inserting the created constraints
    for (auto& node : domTree->GetNodes()) {
        for (auto& constraint : node->nodeConstraints) {
            constraintGraph.addConstraint(constraint);
#ifdef DEBUG_SHOW_INSERTED_CONSTRAINTS
            std::cerr << constraint << std::endl;
#endif
        }
    }
}

// For each function Apply, bind the identifiers of the source function
// to the target function parameters with phi functions.
void RangeAnalysis::BindArgumentsToParamsWithPhiConstraint() {
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

            ValueType valueType;
            auto paramType = params[i]->GetType();
            if (paramType->IsNumeric()) {
                valueType = ValueType::IVType;
            } else if (paramType->IsBoolean()) {
                valueType = ValueType::BVType;
            } else {
                continue;  // Unit or unsupported type
            }

            auto phi = std::make_shared<PhiConstraint>(alias.to_string(), ops,
                                                       valueType);
            constraintGraph.addConstraint(phi);

#ifdef DEBUG_SHOW_INSERTED_CONSTRAINTS
            std::cerr << *phi << std::endl;
#endif
        }
    }
}

// For each return value in target function, bind it to the call result with a
// phi function
void RangeAnalysis::BindReturnValuesToCallResultsWithPhiConstraint() {
    for (auto& [fnName, domTree] : domTree_by_fnName) {
        // Debugging the returnAliases
        for (auto [callee, vals] : domTree->GetReturnAliasMap()) {
            if (!domTree_by_fnName.count(callee)) continue;

            std::vector<std::string> ops;
            auto& calleeTree = domTree_by_fnName[callee];
            for (auto rv : calleeTree->GetReturnValues()) {
                ops.push_back(rv.to_string());
            }

            for (auto& val : vals) {
                ValueType valueType;
                auto retType = calleeTree->GetReturnType();
                if (retType->IsNumeric()) {
                    valueType = ValueType::IVType;
                } else if (retType->IsBoolean()) {
                    valueType = ValueType::BVType;
                } else {
                    continue;  // Unit or unsupported type
                }

                auto phi = std::make_shared<PhiConstraint>(val.to_string(), ops,
                                                           valueType);

                constraintGraph.addConstraint(phi);
#ifdef DEBUG_SHOW_INSERTED_CONSTRAINTS
                std::cerr << *phi << std::endl;
#endif
            }
        }
    }
}

void RangeAnalysis::CreateHelperConstraints() {
    auto cst_0 = std::make_shared<InitializationConstraint>("\%const_0", 0);
    auto cst_true =
        std::make_shared<InitializationBoolConstraint>("\%const_true", true);
    constraintGraph.addConstraint(cst_0);
    constraintGraph.addConstraint(cst_true);
}

// TODO: Accumulate the ranges
static uint32_t CalculateRangeReduction(IV iv) {
    const uint32_t FULL_RANGE = 64;

    // Required number of bits to represent n values.
    auto ceil_log2 = [](std::size_t n) -> uint32_t {
        uint32_t result = 0;
        if (n == 0) return result;
        n--;
        while (n > 0) {
            result++;
            n >>= 1;
        }
        return result;
    };

    if (iv.isBottom()) {  // Full range [-inf, +inf]
        return FULL_RANGE;
    }

    if (iv.getKind() == IV::Kind::Set) {
        // ? # of bits to store that many elements?
        auto el_count = iv.getValues().size();
        return ceil_log2(el_count);
    }

    // iv : IV::King::StridedInterval
    auto low = iv.getLower();
    auto up = iv.getUpper();

    if (low.isConstant() && up.isConstant()) {  // [c1, c2]
        auto c1 = low.getConstant();
        auto c2 = up.getConstant();
        return ceil_log2(c2 - c1 + 1);
    }

    if (low.isConstant()) {  // [c,+inf]
        auto c = low.getConstant();
        return ceil_log2(LONG_MAX - c + 1);
    }

    if (up.isConstant()) {  // [-inf, c]
        auto c = up.getConstant();
        return ceil_log2(c - LONG_MIN + 1);
    }

    // [-inf, +inf]
    return FULL_RANGE;
}

static void RunRangeReductionUnitTests() {
#define TEST_CASE_MK(INIT_CODE, EXPECT)                                   \
    {                                                                     \
        IV iv;                                                            \
        INIT_CODE;                                                        \
        auto actual = CalculateRangeReduction(iv);                        \
        auto passed = actual == EXPECT;                                   \
        if (passed) {                                                     \
            std::cerr << "Test " << test_id << " Passed" << std::endl;    \
        } else {                                                          \
            std::cerr << "Test " << test_id << " Failed" << std::endl;    \
            std::cerr << "\tExpected " << EXPECT << " but got " << actual \
                      << std::endl;                                       \
        }                                                                 \
        test_id++;                                                        \
    }

    int test_id = 1;
    TEST_CASE_MK(iv.setAsBottom(), 64);
    TEST_CASE_MK(auto b = Bound::constant(0); iv.setAsInterval(b, b), 0);
    TEST_CASE_MK(auto l = Bound::constant(0); auto r = Bound::constant(1023);
                 iv.setAsInterval(l, r), 10);
    TEST_CASE_MK(auto l = Bound::constant(0); auto r = Bound::constant(1024);
                 iv.setAsInterval(l, r), 11);
    TEST_CASE_MK(auto l = Bound::constant(0); auto r = Bound::plusInfinity();
                 iv.setAsInterval(l, r), 63);
    TEST_CASE_MK(auto l = Bound::minusInfinity(); auto r = Bound::constant(-1);
                 iv.setAsInterval(l, r), 63);

#undef TEST_CASE_MK
}

void RangeAnalysis::OutputAnalysisToFile() {
    std::fstream outputFile;
    outputFile.open("output.txt", std::ios::out);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output.txt file!" << std::endl;
        return;
    }

    uint32_t bits_needed = 0;
    uint32_t total_bits = 0;

    for (int i = 0; i < queries.size(); i++) {
        if (!queryToDomTree[i].has_value()) {
            std::cerr << "No domTree was found for query!" << std::endl;
            // ? Output bottom range here.
            IV iv;
            iv.setAsBottom();
            outputFile << iv << std::endl;
            bits_needed += CalculateRangeReduction(iv);
            total_bits += 64;
            continue;
        }

        DominatorTree* domTree = queryToDomTree[i].value();

        auto& [fileName, lineNumber, variableName] = queries[i];

#ifdef DEBUG_PRINT_QUERIES
        std::cerr << "Find the range of variable " << variableName
                  << " at line " << lineNumber << " of file " << fileName
                  << std::endl;
#endif

        auto maybeVariableAlias =
            domTree->FindVarBeforeLine(variableName, lineNumber);
        if (!maybeVariableAlias.has_value()) {
            std::cerr << "No alias for variable \"" << variableName
                      << "\" was found for query before line " << lineNumber
                      << "!" << std::endl;
            // ? Output bottom range here.
            IV iv;
            iv.setAsBottom();
            outputFile << iv << std::endl;
            bits_needed += CalculateRangeReduction(iv);
            total_bits += 64;
            continue;
        }

        Alias variableAlias = maybeVariableAlias.value();

#ifdef DEBUG_PRINT_QUERIES
        std::cerr << "You want " << variableAlias.to_string() << ", ";
#endif

        AnalyzedValue variableValue = solverState[variableAlias.to_string()];

        // ? For now just using the default range => no info
        if (std::holds_alternative<BV>(variableValue)) {
            auto boolVal = std::get<BV>(variableValue);
            outputFile << boolVal << std::endl;

#ifdef DEBUG_PRINT_QUERIES
            std::cerr << "Boolean range: " << boolVal << std::endl;
#endif

        } else {
            auto intVal = std::get<IV>(variableValue);
            outputFile << intVal << std::endl;
            bits_needed += CalculateRangeReduction(intVal);
            total_bits += 64;
#ifdef DEBUG_PRINT_QUERIES
            std::cerr << "Integer range: " << intVal << std::endl;
#endif
        }
    }

    std::cerr << "Out of " << total_bits << " only " << bits_needed
              << " were required to represent the Int64 values of this program."
              << std::endl;
    double reduction = 100 * (1.0 - bits_needed / (double)total_bits);
    std::cerr << "Reduction: " << reduction << "%" << std::endl;

    outputFile.close();
}

void RangeAnalysis::RunOnPackage(Package* package) {
    // Filter out the builtin cangjie code
    if (package->GetName() == "std.core") return;

    // Reads input file for value range queries
    ReadCompetitionQueries();
    if (queries.size() < 1) return;

    queryToDomTree.resize(queries.size());

#ifdef DEBUG_PRINT_QUERIES
    std::cerr << "@@@@ COMPETITION ANALYSIS @@@@" << std::endl;
#endif

    GatherRequestedFunctions(package);

    // Compute dominator tree for each function, insert the intraprocedural
    // constraints
    for (auto func : requestedFunctions) BuildDomTreeWithConstraints(func);

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

#ifdef DEBUG_PRINT_QUERIES
    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
#endif
    return;
}

}  // namespace Competition

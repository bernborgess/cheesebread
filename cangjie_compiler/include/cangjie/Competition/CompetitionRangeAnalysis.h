#ifndef COMPETITION_RANGE_ANALYSIS_H
#define COMPETITION_RANGE_ANALYSIS_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cangjie/CHIR/IR/Package.h"
#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/Competition/DominatorTree.h"
#include "cangjie/Competition/RangeAnalysisSolver/Graph.h"

namespace Competition {

struct Query {
    std::string fileName;
    unsigned int lineNumber;
    std::string variableName;
};

class RangeAnalysis {
public:
    RangeAnalysis() { }

    void RunOnPackage(Cangjie::CHIR::Package* package);

private:
    std::vector<Competition::Query> queries;
    void ReadCompetitionQueries();
    
    std::set<Cangjie::CHIR::Function*> requestedFunctions;
    void GatherRequestedFunctions(Cangjie::CHIR::Package* package);

    std::unordered_map<std::string, DominatorTree*> domTree_by_fnName;
    std::vector<std::optional<DominatorTree*>> queryToDomTree;
    void BuildDomTreeWithConstraints(Cangjie::CHIR::Function *func);
    void BindArgumentsToParamsWithPhiConstraint();
    void BindReturnValuesToCallResultsWithPhiConstraint();

    AbstractState solverState;
    ConstraintGraph constraintGraph;
    void CreateHelperConstraints();

    void OutputAnalysisToFile();
};

} // namespace Competition

#endif
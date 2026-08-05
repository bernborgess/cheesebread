#ifndef COMPETITION_RANGE_ANALYSIS_H
#define COMPETITION_RANGE_ANALYSIS_H

#include "cangjie/CHIR/IR/Package.h"
#include "cangjie/CHIR/IR/Value/Value.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Competition {

class RangeAnalysis {
public:
    RangeAnalysis()
    {
    }

    void RunOnPackage(Cangjie::CHIR::Package* package);

private:
    // ! TODO: Also keep track of fileName
    //                 lineNumber => Block*[]
    std::unordered_map<unsigned int, std::vector<Cangjie::CHIR::Block*>>
        blocksByLineNumber;

    void GatherUsefulBasicBlocks(
        std::unordered_set<unsigned int>& interestingLineNumbers,
        std::vector<Cangjie::CHIR::Function*>& userDefinedFunctions
    );
};

} // namespace Competition

#endif
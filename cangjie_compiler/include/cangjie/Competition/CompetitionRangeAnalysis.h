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
};

} // namespace Competition

#endif
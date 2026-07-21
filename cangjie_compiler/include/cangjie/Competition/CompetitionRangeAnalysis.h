#ifndef COMPETITION_RANGE_ANALYSIS_H
#define COMPETITION_RANGE_ANALYSIS_H

#include "cangjie/CHIR/IR/Package.h"

namespace Competition {

class RangeAnalysis {
public:
    RangeAnalysis()
    {
    }

    void RunOnPackage(Cangjie::CHIR::Package*package);
    
};

} // namespace Competition

#endif
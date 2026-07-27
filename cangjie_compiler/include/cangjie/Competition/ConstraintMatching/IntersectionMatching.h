#ifndef INTERSECTION_MATCHING_H
#define INTERSECTION_MATCHING_H

#include <optional>
#include <utility>

#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"

namespace Matching {

using namespace Cangjie::CHIR;

std::pair<std::optional<IntersectionConstraint*>,
          std::optional<IntersectionConstraint*>>
MatchLtVarConst(Value* value);

}  // namespace Matching

#endif

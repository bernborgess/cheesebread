#ifndef INTERSECTION_MATCHING_H
#define INTERSECTION_MATCHING_H

#include <optional>
#include <utility>

#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/Competition/Phi.h"
#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"

namespace Matching {

using namespace Cangjie::CHIR;

struct MatchedConstraints {
    std::vector<IntersectionConstraint*> ifTrue;
    std::vector<IntersectionConstraint*> ifFalse;
};

// x , y : Variables
// c : Constant
//
// if (x < c)
// if (c < x)
// if (x < y)
MatchedConstraints MatchLessThanConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias);

// x , y : Variables
// c : Constant
//
// if (x > c)
// if (c > x)
// if (x > y)
MatchedConstraints MatchGreaterThanConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias);

// x , y : Variables
// c : Constant
//
// if (x == c)
// if (c == x)
// if (x == y)
MatchedConstraints MatchEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias);

// x , y : Variables
// c : Constant
//
// if (x != c)
// if (c != x)
// if (x != y)
MatchedConstraints MatchNotEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias);

// x , y : Variables
// c : Constant
//
// if (x <= c)
// if (c <= x)
// if (x <= y)
MatchedConstraints MatchLessEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias);

// x , y : Variables
// c : Constant
//
// if (x >= c)
// if (c >= x)
// if (x >= y)
MatchedConstraints MatchGreaterEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias);

} // namespace Matching

#endif

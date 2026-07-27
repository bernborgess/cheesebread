#ifndef INTERSECTION_MATCHING_H
#define INTERSECTION_MATCHING_H

#include <optional>
#include <utility>

#include "cangjie/CHIR/IR/Value/Value.h"
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
MatchedConstraints MatchLessThanConstraints(Value* cond);

// x , y : Variables
// c : Constant
// 
// if (x > c)
// if (c > x)
// if (x > y)
MatchedConstraints MatchGreaterThanConstraints(Value* cond);

// x , y : Variables
// c : Constant
// 
// if (x == c)
// if (c == x)
// if (x == y)
MatchedConstraints MatchEqualConstraints(Value* cond);

// x , y : Variables
// c : Constant
// 
// if (x != c)
// if (c != x)
// if (x != y)
MatchedConstraints MatchNotEqualConstraints(Value* cond);

// x , y : Variables
// c : Constant
// 
// if (x <= c)
// if (c <= x)
// if (x <= y)
MatchedConstraints MatchLessEqualConstraints(Value* cond);

// TODO: if (x >= c)
// TODO: if (c >= x)
// TODO: if (x >= y)


}  // namespace Matching

#endif

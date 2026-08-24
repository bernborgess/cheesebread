#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"

#include <variant>

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"

// Match a pattern to create intersection constraints for True and False of a
// Branch
namespace Matching {

using namespace Cangjie::CHIR;

using VarParamOrConstant = std::variant<LocalVar*, Parameter*, Constant*>;

// Gets x from either a Load(x), Parameter(x) or Constant(x)
std::optional<VarParamOrConstant> GetLoadParamOrConstant(Value* value)
{
    if (value->IsLocalVar()) {
        LocalVar* localVar = (LocalVar*)value;
        Expression* expr = localVar->GetExpr();
        if (expr->IsLoad()) {
            Load* load = (Load*)expr;
            Value* locationValue = load->GetLocation();
            if (locationValue->IsLocalVar()) {
                LocalVar* locationLocalVar = (LocalVar*)locationValue;

                // ? Found the load variable
                return locationLocalVar;
            }
        } else if (expr->IsConstant()) {
            Constant* constant = (Constant*)expr;

            // ? Found the constant literal
            return constant;
        }
    } else if (value->IsParameter()) {
        Parameter* param = dynamic_cast<Parameter*>(value);
        return param;
    }

    // ? Nothing to be found
    return {};
}

// Matches the binary operator and returns its left and right arguments
std::pair<std::optional<VarParamOrConstant>,
          std::optional<VarParamOrConstant>>
MatchBinExpr(Value* cond, ExprKind exprKind)
{
    if (cond->IsLocalVar()) {
        LocalVar* condLV = (LocalVar*)cond;
        Expression* condExpr = condLV->GetExpr();
        if (condExpr->GetExprMajorKind() == ExprMajorKind::BINARY_EXPR) {
            BinaryExpression* condBinExpr = (BinaryExpression*)condExpr;
            if (condBinExpr->GetExprKind() == exprKind) {
                return {GetLoadParamOrConstant(condBinExpr->GetLHSOperand()),
                        GetLoadParamOrConstant(condBinExpr->GetRHSOperand())};
            }
        }
    }
    return {{}, {}};
}

static IntersectionConstraint::Future ft(std::string x, int offset = 0)
{
    return IntersectionConstraint::Future{x, offset};
}

// Returns the string that serves as key in idToAlias when phis are used later.
static std::optional<std::string> MatchIdentifierFromValue(VarParamOrConstant v) {
    if (std::holds_alternative<LocalVar*>(v)) {
        auto lv = std::get<LocalVar*>(v);
        return lv->GetSrcCodeIdentifier() != "" ? lv->GetSrcCodeIdentifier()
                                                : lv->GetIdentifier();
    }
    if(std::holds_alternative<Parameter*>(v)) {
        auto param = std::get<Parameter*>(v);
        return param->GetSrcCodeIdentifier();
    }
    return {};
}

// LT(Load(x), Constant(c))
MatchedConstraints MatchLessThanConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::LT);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (auto vx = MatchIdentifierFromValue(matchLHS.value())) {
        // Get x in If-Basic-Block
        auto x = vx.value();

        // if (x < y)
        if (auto vy = MatchIdentifierFromValue(matchRHS.value())) {
            auto y = vy.value();

            // THEN: x = x ∩ [-inf, ft(y) - 1]
            auto trueConstraintX = new IntersectionConstraint(x, x, minusInf, ft(y, -1));
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [ft(x) + 1, +inf]
            auto trueConstraintY = new IntersectionConstraint(y, y, ft(x, 1), plusInf);
            ifTrue.push_back(trueConstraintY);

            // ELSE: x = x ∩ [ft(y), +inf]
            auto falseConstraintX = new IntersectionConstraint(x, x, ft(y), plusInf);
            ifFalse.push_back(falseConstraintX);

            // ELSE: y = y ∩ [-inf, ft(x)]
            auto falseConstraintY = new IntersectionConstraint(y, y, minusInf, ft(x));
            ifFalse.push_back(falseConstraintY);

        } else { // if (x < c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [-inf, c - 1]
            auto trueConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c - 1));
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [c, +inf]
            auto falseConstraint = new IntersectionConstraint(x, x, Bound::constant(c), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c < x)
        if (auto vvx = MatchIdentifierFromValue(matchRHS.value())) {
            auto x = vvx.value();

            // THEN: x = x ∩ [c + 1, +inf]
            auto trueConstraint = new IntersectionConstraint(x, x, Bound::constant(c + 1), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [-inf, c]
            auto falseConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints MatchGreaterThanConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::GT);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (auto vx = MatchIdentifierFromValue(matchLHS.value())) {
        auto x = vx.value();

        // if (x > y)
        if (auto vy = MatchIdentifierFromValue(matchRHS.value())) {
            auto y = vy.value();

            // THEN: x = x ∩ [ft(y) + 1, +inf]
            auto trueConstraintX = new IntersectionConstraint(x, x, ft(y, -1), plusInf);
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [-inf, ft(x) - 1]
            auto trueConstraintY = new IntersectionConstraint(y, y, minusInf, ft(x, -1));
            ifTrue.push_back(trueConstraintY);

            // ELSE: x = x ∩ [-inf, ft(y)]
            auto falseConstraintX = new IntersectionConstraint(x, x, minusInf, ft(y));
            ifFalse.push_back(falseConstraintX);

            // ELSE: y = y ∩ [ft(x), +inf]
            auto falseConstraintY = new IntersectionConstraint(y, y, ft(x), plusInf);
            ifFalse.push_back(falseConstraintY);

        } else { // if (x > c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [c + 1, +inf]
            auto trueConstraint = new IntersectionConstraint(x, x, Bound::constant(c + 1), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [-inf, c]
            auto falseConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c > x)
        if (auto vvx = MatchIdentifierFromValue(matchRHS.value())) {
            auto x = vvx.value();

            // THEN: x = x ∩ [-inf, c - 1]
            auto trueConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c - 1));
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [c, +inf]
            auto falseConstraint = new IntersectionConstraint(x, x, Bound::constant(c), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints MatchEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::EQUAL);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    // Equality is commutative
    if (std::holds_alternative<Constant*>(matchLHS.value())) {
        std::swap(matchLHS, matchRHS);
    }

    if (auto vx = MatchIdentifierFromValue(matchLHS.value())) {
        auto x = vx.value();

        // if (x == y)
        if (auto vy = MatchIdentifierFromValue(matchRHS.value())) {
            auto y = vy.value();

            // THEN: x = x ∩ [ft(y), ft(y)]
            auto trueConstraintX = new IntersectionConstraint(x, x, ft(y, 0), ft(y, 0));
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [ft(x), ft(x)]
            auto trueConstraintY = new IntersectionConstraint(y, y, ft(x, 0), ft(x, 0));
            ifTrue.push_back(trueConstraintY);

            // ELSE: can't build disjoint range.
        } else { // if (x == c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [c, c]
            auto trueConstraint = new IntersectionConstraint(x, x, Bound::constant(c), Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: can't build disjoint range.
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints MatchNotEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::NOTEQUAL);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    // Inequality is commutative
    if (std::holds_alternative<Constant*>(matchLHS.value())) {
        std::swap(matchLHS, matchRHS);
    }

    if (auto vx = MatchIdentifierFromValue(matchLHS.value())) {
        auto x = vx.value();

        // if (x != y)
        if (auto vy = MatchIdentifierFromValue(matchRHS.value())) {
            auto y = vy.value();

            // THEN: can't build disjoint range.

            // ELSE: x = x ∩ [ft(y), ft(y)]
            auto falseConstraintX = new IntersectionConstraint(x, x, ft(y, 0), ft(y, 0));
            ifFalse.push_back(falseConstraintX);

            // ELSE: y = y ∩ [ft(x), ft(x)]
            auto falseConstraintY = new IntersectionConstraint(y, y, ft(x, 0), ft(x, 0));
            ifFalse.push_back(falseConstraintY);

        } else { // if (x != c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: can't build disjoint range.

            // ELSE: x = x ∩ [c, c]
            auto falseConstraint = new IntersectionConstraint(x, x, Bound::constant(c), Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints MatchLessEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::LE);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (auto vx = MatchIdentifierFromValue(matchLHS.value())) {
        auto x = vx.value();

        // if (x <= y)
        if (auto vy = MatchIdentifierFromValue(matchRHS.value())) {
            auto y = vy.value();

            // THEN: x = x ∩ [-inf, ft(y)]
            auto trueConstraintX = new IntersectionConstraint(x, x, minusInf, ft(y));
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [ft(x), +inf]
            auto trueConstraintY = new IntersectionConstraint(y, y, ft(x), plusInf);
            ifTrue.push_back(trueConstraintY);

            // ELSE: x = x ∩ [ft(y) + 1, +inf]
            auto falseConstraintX = new IntersectionConstraint(x, x, ft(y, 1), plusInf);
            ifFalse.push_back(falseConstraintX);

            // ELSE: y = y ∩ [-inf, ft(x) - 1]
            auto falseConstraintY = new IntersectionConstraint(y, y, minusInf, ft(x, -1));
            ifFalse.push_back(falseConstraintY);
        } else { // if (x <= c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [-inf, c]
            auto trueConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [c + 1, +inf]
            auto falseConstraint = new IntersectionConstraint(x, x, Bound::constant(c + 1), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c <= x)
        if (auto vvx = MatchIdentifierFromValue(matchRHS.value())) {
            auto x = vvx.value();

            // THEN: x = x ∩ [c, +inf]
            auto trueConstraint = new IntersectionConstraint(x, x, Bound::constant(c), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [-inf, c - 1]
            auto falseConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c - 1));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints MatchGreaterEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::GE);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (auto vx = MatchIdentifierFromValue(matchLHS.value())) {
        auto x = vx.value();

        // if (x >= y)
        if (auto vy = MatchIdentifierFromValue(matchRHS.value())) {
            auto y = vy.value();

            // THEN: x = x ∩ [ft(y), +inf]
            auto trueConstraintX = new IntersectionConstraint(x, x, ft(y), plusInf);
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [-inf, ft(x)]
            auto trueConstraintY = new IntersectionConstraint(y, y, minusInf, ft(x));
            ifTrue.push_back(trueConstraintY);

            // ELSE: x = x ∩ [-inf, ft(y) - 1]
            auto falseConstraintX = new IntersectionConstraint(x, x, minusInf, ft(y, -1));
            ifFalse.push_back(falseConstraintX);

            // ELSE: y = y ∩ [ft(x) + 1, +inf]
            auto falseConstraintY = new IntersectionConstraint(y, y, ft(x, 1), plusInf);
            ifFalse.push_back(falseConstraintY);

        } else { // if (x >= c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [c, +inf]
            auto trueConstraint = new IntersectionConstraint(x, x, Bound::constant(c), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [-inf, c - 1]
            auto falseConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c - 1));
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c >= x)
        if (auto vvx = MatchIdentifierFromValue(matchRHS.value())) {
            auto x = vvx.value();

            // THEN: x = x ∩ [-inf, c]
            auto trueConstraint = new IntersectionConstraint(x, x, minusInf, Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [c + 1, +inf]
            auto falseConstraint = new IntersectionConstraint(x, x, Bound::constant(c + 1), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

} // namespace Matching
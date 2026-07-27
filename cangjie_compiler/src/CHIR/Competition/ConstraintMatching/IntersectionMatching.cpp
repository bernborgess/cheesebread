#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"

#include <variant>

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"

// Match a pattern to create intersection constraints for True and False of a
// Branch
namespace Matching {

using namespace Cangjie::CHIR;

// Gets x from either a Load(x) or Constant(x)
std::optional<std::variant<LocalVar*, Constant*>> GetLoadOrConstant(
    Value* value) {
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
    }

    // ? Nothing to be found
    return {};
}

// Matches the binary operator and returns its left and right arguments
std::pair<std::optional<std::variant<LocalVar*, Constant*>>,
          std::optional<std::variant<LocalVar*, Constant*>>>
MatchBinExpr(Value* cond, ExprKind exprKind) {
    if (cond->IsLocalVar()) {
        LocalVar* condLV = (LocalVar*)cond;
        Expression* condExpr = condLV->GetExpr();
        if (condExpr->GetExprMajorKind() == ExprMajorKind::BINARY_EXPR) {
            BinaryExpression* condBinExpr = (BinaryExpression*)condExpr;
            if (condBinExpr->GetExprKind() == exprKind) {
                return {GetLoadOrConstant(condBinExpr->GetLHSOperand()),
                        GetLoadOrConstant(condBinExpr->GetRHSOperand())};
            }
        }
    }
    return {{}, {}};
}

// LT(Load(x), Constant(c))
MatchedConstraints Matching::MatchLessThanConstraints(Value* cond) {
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::LT);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x = std::get<LocalVar*>(matchLHS.value())->GetSrcCodeIdentifier();

        auto ft = [](std::string x, int offset = 0) {
            return IntersectionConstraint::Future{x, offset};
        };

        // if (x < y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y =
                std::get<LocalVar*>(matchRHS.value())->GetSrcCodeIdentifier();

            // THEN: x = x ∩ [-inf, ft(y) - 1]
            auto trueConstraintX =
                new IntersectionConstraint(x, x, minusInf, ft(y, -1));
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [ft(x) + 1, +inf]
            auto trueConstraintY =
                new IntersectionConstraint(y, y, ft(x, 1), plusInf);
            ifTrue.push_back(trueConstraintY);

            // ELSE: x = x ∩ [ft(y), +inf]
            auto falseConstraintX =
                new IntersectionConstraint(x, x, ft(y), plusInf);
            ifFalse.push_back(falseConstraintX);

            // ELSE: y = y ∩ [-inf, ft(x)]
            auto falseConstraintY =
                new IntersectionConstraint(y, y, minusInf, ft(x));
            ifFalse.push_back(falseConstraintY);

        } else {  // if (x < c)
            auto c =
                std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [-inf, c - 1]
            auto trueConstraint = new IntersectionConstraint(
                x, x, minusInf, Bound::constant(c - 1));
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [c, +inf]
            auto falseConstraint =
                new IntersectionConstraint(x, x, Bound::constant(c), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c < x)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto x =
                std::get<LocalVar*>(matchRHS.value())->GetSrcCodeIdentifier();

            // THEN: x = x ∩ [c + 1, +inf]
            auto trueConstraint = new IntersectionConstraint(
                x, x, Bound::constant(c + 1), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x = x ∩ [-inf, c]
            auto falseConstraint =
                new IntersectionConstraint(x, x, minusInf, Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints Matching::MatchEqualConstraints(Value* cond) {
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::EQUAL);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    // Equality is commutative
    if (std::holds_alternative<Constant*>(matchLHS.value())) {
        std::swap(matchLHS, matchRHS);
    }

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x = std::get<LocalVar*>(matchLHS.value())->GetSrcCodeIdentifier();

        auto ft = [](std::string x, int offset = 0) {
            return IntersectionConstraint::Future{x, offset};
        };

        // if (x == y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y =
                std::get<LocalVar*>(matchRHS.value())->GetSrcCodeIdentifier();

            // THEN: x = x ∩ [ft(y), ft(y)]
            auto trueConstraintX =
                new IntersectionConstraint(x, x, ft(y, 0), ft(y, 0));
            ifTrue.push_back(trueConstraintX);

            // THEN: y = y ∩ [ft(x), ft(x)]
            auto trueConstraintY =
                new IntersectionConstraint(y, y, ft(x, 0), ft(x, 0));
            ifTrue.push_back(trueConstraintY);

            // ELSE: can't build disjoint range.
        } else {  // if (x == c)
            auto c =
                std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x = x ∩ [c, c]
            auto trueConstraint = new IntersectionConstraint(
                x, x, Bound::constant(c), Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: can't build disjoint range.
        }
    }

    return {ifTrue, ifFalse};
}

}  // namespace Matching
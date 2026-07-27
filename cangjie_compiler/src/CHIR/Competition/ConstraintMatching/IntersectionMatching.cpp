#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"

#include <variant>

// Match a pattern to create intersection constraints for True and False of a
// Branch

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
        } else if(expr->IsConstant()) {
            Constant* constant = (Constant*)expr;

            // ? Found the constant literal
            return constant;
        }
    }

    // ? Nothing to be found
    return {};
}

// LT(Load(x), Constant(c))
Matching::MatchedConstraints Matching::MatchLessThanConstraints(Value* cond) {
    std::optional<std::variant<LocalVar*, Constant*>> matchLHS = std::nullopt,
                                                      matchRHS = std::nullopt;

    if (cond->IsLocalVar()) {
        LocalVar* condLV = (LocalVar*)cond;
        Expression* condExpr = condLV->GetExpr();
        if (condExpr->GetExprMajorKind() == ExprMajorKind::BINARY_EXPR) {
            BinaryExpression* condBinExpr = (BinaryExpression*)condExpr;
            if (condBinExpr->GetExprKind() == ExprKind::LT) {
                matchLHS = GetLoadOrConstant(condBinExpr->GetLHSOperand());
                matchRHS = GetLoadOrConstant(condBinExpr->GetRHSOperand());
            }
        }
    }

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
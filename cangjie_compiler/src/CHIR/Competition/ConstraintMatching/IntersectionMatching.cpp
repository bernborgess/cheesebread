#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"

#include <variant>

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"

// Match a pattern to create intersection constraints for True and False of a
// Branch
namespace Matching {

using namespace Cangjie::CHIR;

// Gets x from either a Load(x) or Constant(x)
std::optional<std::variant<LocalVar*, Constant*>> GetLoadOrConstant(Value* value)
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
    }

    // ? Nothing to be found
    return {};
}

// Matches the binary operator and returns its left and right arguments
std::pair<std::optional<std::variant<LocalVar*, Constant*>>,
          std::optional<std::variant<LocalVar*, Constant*>>>
MatchBinExpr(Value* cond, ExprKind exprKind)
{
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

static IntersectionConstraint::Future ft(std::string x, int offset = 0)
{
    return IntersectionConstraint::Future{x, offset};
}

// LT(Load(x), Constant(c))
MatchedConstraints Matching::MatchLessThanConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::LT);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        // Get x in If-Basic-Block
        auto x_alias = idToAlias.at(std::get<LocalVar*>(matchLHS.value())->GetIdentifier());
        auto x = x_alias.to_string();
        // Create new alias for the branching
        auto x_then = "\%t_" + x;
        auto x_else = "\%f_" + x;

        // if (x < y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto y = y_alias.to_string();
            auto y_then = "\%t_" + y;
            auto y_else = "\%f_" + y;

            // THEN: x_t = x ∩ [-inf, ft(y) - 1]
            auto trueConstraintX = new IntersectionConstraint(x_then, x, minusInf, ft(y, -1));
            ifTrue.push_back(trueConstraintX);

            // THEN: y_t = y ∩ [ft(x) + 1, +inf]
            auto trueConstraintY = new IntersectionConstraint(y_then, y, ft(x, 1), plusInf);
            ifTrue.push_back(trueConstraintY);

            // ELSE: x_f = x ∩ [ft(y), +inf]
            auto falseConstraintX = new IntersectionConstraint(x_else, x, ft(y), plusInf);
            ifFalse.push_back(falseConstraintX);

            // ELSE: y_f = y ∩ [-inf, ft(x)]
            auto falseConstraintY = new IntersectionConstraint(y_else, y, minusInf, ft(x));
            ifFalse.push_back(falseConstraintY);

        } else { // if (x < c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x_t = x ∩ [-inf, c - 1]
            auto trueConstraint = new IntersectionConstraint(x_then, x, minusInf, Bound::constant(c - 1));
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [c, +inf]
            auto falseConstraint = new IntersectionConstraint(x_else, x, Bound::constant(c), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c < x)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto x_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto x = x_alias.to_string();
            auto x_then = "\%t_" + x;
            auto x_else = "\%f_" + x;

            // THEN: x_t = x ∩ [c + 1, +inf]
            auto trueConstraint = new IntersectionConstraint(x_then, x, Bound::constant(c + 1), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [-inf, c]
            auto falseConstraint = new IntersectionConstraint(x_else, x, minusInf, Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints Matching::MatchGreaterThanConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::GT);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x_alias = idToAlias.at(std::get<LocalVar*>(matchLHS.value())->GetIdentifier());
        auto x = x_alias.to_string();
        auto x_then = "\%t_" + x;
        auto x_else = "\%f_" + x;

        // if (x > y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto y = y_alias.to_string();
            auto y_then = "\%t_" + y;
            auto y_else = "\%f_" + y;

            // THEN: x_t = x ∩ [ft(y) + 1, +inf]
            auto trueConstraintX = new IntersectionConstraint(x_then, x, ft(y, -1), plusInf);
            ifTrue.push_back(trueConstraintX);

            // THEN: y_t = y ∩ [-inf, ft(x) - 1]
            auto trueConstraintY = new IntersectionConstraint(y_then, y, minusInf, ft(x, -1));
            ifTrue.push_back(trueConstraintY);

            // ELSE: x_f = x ∩ [-inf, ft(y)]
            auto falseConstraintX = new IntersectionConstraint(x_else, x, minusInf, ft(y));
            ifFalse.push_back(falseConstraintX);

            // ELSE: y_f = y ∩ [ft(x), +inf]
            auto falseConstraintY = new IntersectionConstraint(y_else, y, ft(x), plusInf);
            ifFalse.push_back(falseConstraintY);

        } else { // if (x > c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x_t = x ∩ [c + 1, +inf]
            auto trueConstraint = new IntersectionConstraint(x_then, x, Bound::constant(c + 1), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [-inf, c]
            auto falseConstraint = new IntersectionConstraint(x_else, x, minusInf, Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c > x)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto x_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto x = x_alias.to_string();
            auto x_then = "\%t_" + x;
            auto x_else = "\%f_" + x;

            // THEN: x_t = x ∩ [-inf, c - 1]
            auto trueConstraint = new IntersectionConstraint(x_then, x, minusInf, Bound::constant(c - 1));
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [c, +inf]
            auto falseConstraint = new IntersectionConstraint(x_else, x, Bound::constant(c), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints Matching::MatchEqualConstraints(
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

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x_alias = idToAlias.at(std::get<LocalVar*>(matchLHS.value())->GetIdentifier());
        auto x = x_alias.to_string();
        auto x_then = "\%t_" + x;

        // if (x == y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto y = y_alias.to_string();
            auto y_then = "\%t_" + y;

            // THEN: x_t = x ∩ [ft(y), ft(y)]
            auto trueConstraintX = new IntersectionConstraint(x_then, x, ft(y, 0), ft(y, 0));
            ifTrue.push_back(trueConstraintX);

            // THEN: y_t = y ∩ [ft(x), ft(x)]
            auto trueConstraintY = new IntersectionConstraint(y_then, y, ft(x, 0), ft(x, 0));
            ifTrue.push_back(trueConstraintY);

            // ELSE: can't build disjoint range.
        } else { // if (x == c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x_t = x ∩ [c, c]
            auto trueConstraint = new IntersectionConstraint(x_then, x, Bound::constant(c), Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: can't build disjoint range.
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints Matching::MatchNotEqualConstraints(
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

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x_alias = idToAlias.at(std::get<LocalVar*>(matchLHS.value())->GetIdentifier());
        auto x = x_alias.to_string();
        auto x_else = "\%f_" + x;

        // if (x != y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto y = y_alias.to_string();
            auto y_else = "\%f_" + y;

            // THEN: can't build disjoint range.

            // ELSE: x_f = x ∩ [ft(y), ft(y)]
            auto falseConstraintX = new IntersectionConstraint(x_else, x, ft(y, 0), ft(y, 0));
            ifFalse.push_back(falseConstraintX);

            // ELSE: y_f = y ∩ [ft(x), ft(x)]
            auto falseConstraintY = new IntersectionConstraint(y_else, y, ft(x, 0), ft(x, 0));
            ifFalse.push_back(falseConstraintY);

        } else { // if (x != c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: can't build disjoint range.

            // ELSE: x_f = x ∩ [c, c]
            auto falseConstraint = new IntersectionConstraint(x_else, x, Bound::constant(c), Bound::constant(c));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints Matching::MatchLessEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::LE);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x_alias = idToAlias.at(std::get<LocalVar*>(matchLHS.value())->GetIdentifier());
        auto x = x_alias.to_string();
        auto x_then = "\%t_" + x;
        auto x_else = "\%f_" + x;

        // if (x <= y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto y = y_alias.to_string();
            auto y_then = "\%t_" + y;
            auto y_else = "\%f_" + y;

            // THEN: x_t = x ∩ [-inf, ft(y)]
            auto trueConstraintX = new IntersectionConstraint(x_then, x, minusInf, ft(y));
            ifTrue.push_back(trueConstraintX);

            // THEN: y_t = y ∩ [ft(x), +inf]
            auto trueConstraintY = new IntersectionConstraint(y_then, y, ft(x), plusInf);
            ifTrue.push_back(trueConstraintY);

            // ELSE: x_f = x ∩ [ft(y) + 1, +inf]
            auto falseConstraintX = new IntersectionConstraint(x_else, x, ft(y, 1), plusInf);
            ifFalse.push_back(falseConstraintX);

            // ELSE: y_f = y ∩ [-inf, ft(x) - 1]
            auto falseConstraintY = new IntersectionConstraint(y_else, y, minusInf, ft(x, -1));
            ifFalse.push_back(falseConstraintY);
        } else { // if (x <= c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x_t = x ∩ [-inf, c]
            auto trueConstraint = new IntersectionConstraint(x_then, x, minusInf, Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [c + 1, +inf]
            auto falseConstraint = new IntersectionConstraint(x_else, x, Bound::constant(c + 1), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c <= x)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto x_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto x = x_alias.to_string();
            auto x_then = "\%t_" + x;
            auto x_else = "\%f_" + x;

            // THEN: x_t = x ∩ [c, +inf]
            auto trueConstraint = new IntersectionConstraint(x_then, x, Bound::constant(c), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [-inf, c - 1]
            auto falseConstraint = new IntersectionConstraint(x_else, x, minusInf, Bound::constant(c - 1));
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

MatchedConstraints Matching::MatchGreaterEqualConstraints(
    Value* cond, const std::unordered_map<std::string, Competition::Alias>& idToAlias)
{
    auto [matchLHS, matchRHS] = MatchBinExpr(cond, ExprKind::GE);

    if (!matchLHS.has_value() || !matchRHS.has_value()) return {{}, {}};

    std::vector<IntersectionConstraint*> ifTrue, ifFalse;

    auto minusInf = Bound::minusInfinity();
    auto plusInf = Bound::plusInfinity();

    if (std::holds_alternative<LocalVar*>(matchLHS.value())) {
        auto x_alias = idToAlias.at(std::get<LocalVar*>(matchLHS.value())->GetIdentifier());
        auto x = x_alias.to_string();
        auto x_then = "\%t_" + x;
        auto x_else = "\%f_" + x;

        // if (x >= y)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto y_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto y = y_alias.to_string();
            auto y_then = "\%t_" + y;
            auto y_else = "\%f_" + y;

            // THEN: x_t = x ∩ [ft(y), +inf]
            auto trueConstraintX = new IntersectionConstraint(x_then, x, ft(y), plusInf);
            ifTrue.push_back(trueConstraintX);

            // THEN: y_t = y ∩ [-inf, ft(x)]
            auto trueConstraintY = new IntersectionConstraint(y_then, y, minusInf, ft(x));
            ifTrue.push_back(trueConstraintY);

            // ELSE: x_f = x ∩ [-inf, ft(y) - 1]
            auto falseConstraintX = new IntersectionConstraint(x_else, x, minusInf, ft(y, -1));
            ifFalse.push_back(falseConstraintX);

            // ELSE: y_f = y ∩ [ft(x) + 1, +inf]
            auto falseConstraintY = new IntersectionConstraint(y_else, y, ft(x, 1), plusInf);
            ifFalse.push_back(falseConstraintY);

        } else { // if (x >= c)
            auto c = std::get<Constant*>(matchRHS.value())->GetSignedIntLitVal();

            // THEN: x_t = x ∩ [c, +inf]
            auto trueConstraint = new IntersectionConstraint(x_then, x, Bound::constant(c), plusInf);
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [-inf, c - 1]
            auto falseConstraint = new IntersectionConstraint(x_else, x, minusInf, Bound::constant(c - 1));
            ifFalse.push_back(falseConstraint);
        }
    } else {
        auto c = std::get<Constant*>(matchLHS.value())->GetSignedIntLitVal();

        // if (c >= x)
        if (std::holds_alternative<LocalVar*>(matchRHS.value())) {
            auto x_alias = idToAlias.at(std::get<LocalVar*>(matchRHS.value())->GetIdentifier());
            auto x = x_alias.to_string();
            auto x_then = "\%t_" + x;
            auto x_else = "\%f_" + x;

            // THEN: x_t = x ∩ [-inf, c]
            auto trueConstraint = new IntersectionConstraint(x_then, x, minusInf, Bound::constant(c));
            ifTrue.push_back(trueConstraint);

            // ELSE: x_f = x ∩ [c + 1, +inf]
            auto falseConstraint = new IntersectionConstraint(x_else, x, Bound::constant(c + 1), plusInf);
            ifFalse.push_back(falseConstraint);
        }
    }

    return {ifTrue, ifFalse};
}

} // namespace Matching
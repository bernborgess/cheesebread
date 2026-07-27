#include "cangjie/Competition/ConstraintMatching/IntersectionMatching.h"

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"

// Match a pattern to create intersection constraints for True and False of a
// Branch

// LT(Load(x), Constant(c))
std::pair<std::optional<IntersectionConstraint*>,
          std::optional<IntersectionConstraint*>>
Matching::MatchLtVarConst(Value* cond) {
    std::optional<std::string> x = std::nullopt;
    std::optional<int64_t> c = std::nullopt;

    if (cond->IsLocalVar()) {
        LocalVar* condLV = (LocalVar*)cond;
        Expression* condExpr = condLV->GetExpr();
        if (condExpr->GetExprMajorKind() == ExprMajorKind::BINARY_EXPR) {
            BinaryExpression* condBinExpr = (BinaryExpression*)condExpr;
            if (condBinExpr->GetExprKind() == ExprKind::LT) {
                Value* lhs = condBinExpr->GetLHSOperand();
                if (lhs->IsLocalVar()) {
                    LocalVar* lhsLV = (LocalVar*)lhs;
                    Expression* lhsExpr = lhsLV->GetExpr();
                    if (lhsExpr->IsLoad()) {
                        Load* lhsLoad = (Load*)lhsExpr;
                        Value* lhsLocation = lhsLoad->GetLocation();
                        if (lhsLocation->IsLocalVar()) {
                            LocalVar* lhsLocationLV = (LocalVar*)lhsLocation;
                            // ! Found the load variable
                            x = lhsLocationLV->GetSrcCodeIdentifier();
                        }
                    }
                }

                Value* rhs = condBinExpr->GetRHSOperand();
                if (rhs->IsLocalVar()) {
                    LocalVar* rhsLV = (LocalVar*)rhs;
                    Expression* rhsExpr = rhsLV->GetExpr();
                    if (rhsExpr->IsConstant()) {
                        Constant* rhsConstant = (Constant*)rhsExpr;
                        // ! Found the constant literal
                        // TODO: Validate ConstLiteralKind is Int
                        c = rhsConstant->GetSignedIntLitVal();
                    }
                }
            }
        }
    }

    if (x.has_value() && c.has_value()) {
        // * Generate constraint on true block
        IntersectionConstraint::IntersectionBound lowTrue =
            Bound::minusInfinity();
        IntersectionConstraint::IntersectionBound upTrue =
            Bound::constant(c.value() - 1);
        IntersectionConstraint* constraintTrue =
            new IntersectionConstraint(x.value(), x.value(), lowTrue, upTrue);

        // * Generate constraint on false block
        IntersectionConstraint::IntersectionBound lowFalse =
            Bound::constant(c.value());
        IntersectionConstraint::IntersectionBound upFalse =
            Bound::plusInfinity();
        IntersectionConstraint* constraintFalse =
            new IntersectionConstraint(x.value(), x.value(), lowFalse, upFalse);

        return {constraintTrue, constraintFalse};
    }

    return {std::nullopt,std::nullopt};
}
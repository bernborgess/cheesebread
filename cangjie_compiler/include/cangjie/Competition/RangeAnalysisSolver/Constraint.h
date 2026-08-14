/**
 * @file Constraint.h
 * @brief Declarations for the SSA Constraint hierarchy.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include "IntValue.h"
#include "BoolValue.h"

enum class EdgeType{
   Data,
   Future,
};

struct UseEdge{
   std::string target_variable;
   EdgeType type;
};

using IV = IntValue<4>;
using BV = BoolValue;
using AnalyzedValue = std::variant<IV, BV>;
// Define our global abstract state table using the alias
using AbstractState = std::unordered_map<std::string, AnalyzedValue>;

/**
 * @class Constraint
 * @brief Abstract base class for all dataflow equations.
 */
class Constraint {
public:
    std::string def;

    explicit Constraint(std::string name);
    virtual ~Constraint() = default;

    /**
     * @brief Evaluates the constraint against the current abstract state.
     * @param A The global variable-to-value map.
     * @return true if the variable_name's value changed, false otherwise.
     */
    virtual bool eval(AbstractState& A) = 0;

    /**
     * @brief Refines the abstract value of a variable using a monotonic
     * narrowing operator.
     * @param A The current abstract state map tracking variable domain
     * evaluations.
     * @return true if the abstract value was successfully narrowed (shrunk),
     * false if the domain remained unchanged (indicating a fixed point).
     */
    bool narrow(AbstractState& A) {
      if (std::holds_alternative<BV>(A[def])) {
        // BoolValue doesn't need narrowing
        return false;
      }

      IV oldY = std::get<IV>(A[def]);

      // force eval()'s bottom-case branch
      std::get<IV>(A[def]).setAsBottom();     
      eval(A);                                 // e(Y)
      IV eY = std::get<IV>(A[def]);

      if (oldY.getKind() == IV::Kind::Set && eY.getKind() == IV::Kind::Set) {
        IV result = oldY;
        std::vector<int> vals;
        for (auto val : eY.getValues()) {
          vals.emplace_back(val);
        }
        result.addConstant(vals);
        A[def] = result;
        return result != oldY;
      }

      Bound lo = oldY.getLower();
      Bound hi = oldY.getUpper();

      // 1. Guard 1: I[Y] is -Infinity, and e(Y) has recovered to a finite bound
      if (oldY.getLower().isMinusInfinity() &&
          !eY.getLower().isMinusInfinity()) {
        lo = eY.getLower();
      }
      // 3. Guard 3: e(Y) lower bound is greater (tighter) than oldY lower
      // bound -> Narrow!
      else if (eY.getLower() > oldY.getLower()) {
        lo = eY.getLower();
      }

      // 2. Guard 2: I[Y] is +Infinity, and e(Y) has recovered to a finite bound
      if (oldY.getUpper().isPlusInfinity() &&
          !eY.getUpper().isPlusInfinity()) {
        hi = eY.getUpper();
      }
      // 4. Guard 4: e(Y) upper bound is smaller (tighter) than oldY upper
      // bound -> Narrow!
      else if (eY.getUpper() < oldY.getUpper()) {
        hi = eY.getUpper();
      }

      std::get<IV>(A[def]).setAsInterval(lo, hi, 1);

      // Termination relies on this returning false when no further shrinking
      // occurs
      return std::get<IV>(A[def]) != oldY;

    }
    
    virtual std::vector<UseEdge> get_uses() const = 0; 
    std::string get_def(){
      return def;
    }

    friend std::ostream& operator<<(std::ostream& os, const Constraint& c) {
        os << c.def;
        return os;
    }
};

/**
 * @class InitializationConstraint
 * @brief Models literal assignments: v = c
 */
class InitializationConstraint : public Constraint {
private:
    int constant;
public:
    InitializationConstraint(std::string var, int c);
    bool eval(AbstractState& A) override;

    std::vector<UseEdge> get_uses() const override {
        std::vector<UseEdge> ret = {};
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const InitializationConstraint& c) {
        os << c.def << ": " << c.constant;
        return os;
    }
};

class InitializationIntegerTop : public Constraint {
public:
    InitializationIntegerTop(std::string var);
    bool eval(AbstractState& A) override;

    std::vector<UseEdge> get_uses() const override {
        std::vector<UseEdge> ret = {};
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const InitializationIntegerTop& c) {
        os << c.def << ": [-inf, +inf]";
        return os;
    }
};

/**
 * @class PhiConstraint
 * @brief Models SSA control-flow merges: v0 = phi(v1, v2, ..., vk)
 */
class PhiConstraint : public Constraint {
private:
    std::vector<std::string> operands;
public:
    PhiConstraint(std::string var, std::vector<std::string> ops);
    bool eval(AbstractState& A) override;

    std::vector<UseEdge> get_uses() const override {
      std::vector<UseEdge> edges;
      edges.reserve(operands.size());

      for(const std::string& op : operands) {
         edges.push_back({op, EdgeType::Data});
      }
      
      return edges;
    }

    friend std::ostream& operator<<(std::ostream& os, const PhiConstraint c) {
        os << c.def << " φ(";
        for (int i = 0; i < c.operands.size(); i++) {
            if (i > 0) os << ", ";
            os << c.operands[i];
        }
        os << ")";
        return os;
    }
};

/**
 * @class IntersectionConstraint
 * @brief Models narrowing blocks: v0 = v1 intersection [low, up]
 * The bound endpoints can either be a static integer constant, an infinity,
 * or a "Future" referencing another variable.
 */
class IntersectionConstraint : public Constraint {
public:
    // A Future represents a symbolic reference to another variable's state
    struct Future {
        std::string target_variable;
        int offset; // Handles relations like Future(y) - 1 or Future(x) + 1
    };

    // An intersection boundary can be a literal Constant, an Infinity, or a
    // Future
    using IntersectionBound = std::variant<Bound, Future>;

    friend std::ostream& operator<<(std::ostream& os, const IntersectionBound b) {
        if (std::holds_alternative<Bound>(b)) {
            os << std::get<Bound>(b);
        } else {
            const Future &f = std::get<Future>(b);
            os << "f(" << f.target_variable << ")";
            if (f.offset > 0) os << " + " << f.offset;
            else if (f.offset < 0) os << " - " << -f.offset;
        }
        return os;
    }

    // @brief Replace symbolic bounds with concrete bounds.
    // @param state The table with abstract states that we will inspect to
    //   resolve symbolic bounds.
    IntersectionConstraint resolveFutures(
      const AbstractState &state) const;

    // Needed in renaming as well
    std::string operand;

private:
    IntersectionBound lower_bound;
    IntersectionBound upper_bound;

    // Helper to resolve a variant bound into a concrete Bound
    // at runtime
    Bound resolveBound(
        const IntersectionBound& b,
        const bool isLower,
        const AbstractState& A
        ) const;

public:
    IntersectionConstraint(std::string dest, std::string src,
                           IntersectionBound low, IntersectionBound up);
    bool eval(AbstractState& A) override;


    std::vector<UseEdge> get_uses() const override {

      std::vector<UseEdge> uses;
      uses.push_back({operand, EdgeType::Data});

      if(std::holds_alternative<Future>(lower_bound)){
         uses.push_back({std::get<Future>(lower_bound).target_variable, EdgeType::Future});
      }

      if(std::holds_alternative<Future>(upper_bound)){
         uses.push_back({std::get<Future>(upper_bound).target_variable, EdgeType::Future});
      }

      return uses;
    }

    friend std::ostream& operator<<(std::ostream& os, const IntersectionConstraint c) {
        os << c.def << " = σ(" << c.operand << " ∩ "
           << "[" << c.lower_bound << "," << c.upper_bound << "])";
        return os;
    }
};

/**
 * @class RangeConstraint
 * @brief Models range-literal assignments over the IntValue domain:
 * v0 = low..high (right-open), v0 = low..=high (closed), optionally with a
 * step: v0 = low..high : step.
 *
 * Endpoints reuse IntersectionConstraint's IntersectionBound so a range edge
 * can be a literal Bound (constant or infinity) or a Future referencing
 * another variable's current bound, exactly like intersection narrowing.
 */
class RangeConstraint : public Constraint {
public:
    using RangeBound = IntersectionConstraint::IntersectionBound;
    using Future = IntersectionConstraint::Future;

private:
    RangeBound low_bound;
    RangeBound high_bound;
    bool inclusive; // false: low..high (right-open) | true: low..=high (closed)
    unsigned step;  // step specification (low..high : step); defaults to 1

    Bound resolveBound(const RangeBound &b, bool isLower,
                       const AbstractState &A) const;

public:
    RangeConstraint(std::string dest, RangeBound low, RangeBound high,
                    bool inclusive, unsigned step = 1)
        : Constraint(std::move(dest)), low_bound(std::move(low)),
          high_bound(std::move(high)), inclusive(inclusive), step(step) {}

    bool eval(AbstractState& A) override;

    std::vector<UseEdge> get_uses() const override {
        std::vector<UseEdge> uses;
        if (std::holds_alternative<Future>(low_bound))
            uses.push_back({std::get<Future>(low_bound).target_variable, EdgeType::Future});
        if (std::holds_alternative<Future>(high_bound))
            uses.push_back({std::get<Future>(high_bound).target_variable, EdgeType::Future});
        return uses;
    }

    friend std::ostream& operator<<(std::ostream& os, const RangeConstraint& c) {
        os << c.def << ": " << c.low_bound << (c.inclusive ? " ..= " : " .. ")
           << c.high_bound;
        if (c.step != 1) os << " : " << c.step;
        return os;
    }
};

/**
 * @class UnaryConstraint
 * @brief Base class for constraints tracking a single operand. Reused both
 * for IntValue-typed unary operations (negation, increment/decrement,
 * bitwise NOT) and for the BoolValue-typed LogicalNotConstraint, exactly
 * like ArithmeticConstraint is already reused for both IV and BV binary
 * constraints below.
 */
class UnaryConstraint : public Constraint {
protected:
    std::string operand;
public:
    UnaryConstraint(std::string dest, std::string src);

    std::vector<UseEdge> get_uses() const override {
        return {{operand, EdgeType::Data}};
    }
};

/**
 * @class ArithmeticConstraint
 * @brief Base class for binary arithmetic constraints tracking two operands.
 */
class ArithmeticConstraint : public Constraint {
protected:
    std::string op1;
    std::string op2;
public:
    ArithmeticConstraint(std::string dest, std::string lhs, std::string rhs);

   std::vector<UseEdge> get_uses() const override{
      return {{op1, EdgeType::Data}, {op2, EdgeType::Data}};
   }
};

/**
 * @class AddConstraint
 * @brief Models abstract addition: v0 = v1 + v2
 */
class AddConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const AddConstraint c) {
        os << c.def << ": " << c.op1 << " + " << c.op2;
        return os;
    }
};

/**
 * @class SubConstraint
 * @brief Models abstract subtraction: v0 = v1 - v2
 */
class SubConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const SubConstraint c) {
        os << c.def << ": " << c.op1 << " - " << c.op2;
        return os;
    }
};

/**
 * @class DivConstraint
 * @brief Models abstract integer (truncating) division: v0 = v1 / v2
 */
class DivConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const DivConstraint c) {
        os << c.def << ": " << c.op1 << " / " << c.op2;
        return os;
    }
};

/**
 * @class ModConstraint
 * @brief Models abstract remainder (C++ truncating semantics): v0 = v1 % v2
 */
class ModConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const ModConstraint c) {
        os << c.def << ": " << c.op1 << " % " << c.op2;
        return os;
    }
};

/**
 * @class ShiftLeftConstraint
 * @brief Models abstract left shift: v0 = v1 << v2
 */
class ShiftLeftConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const ShiftLeftConstraint c) {
        os << c.def << ": " << c.op1 << " << " << c.op2;
        return os;
    }
};

/**
 * @class ShiftRightConstraint
 * @brief Models abstract (arithmetic) right shift: v0 = v1 >> v2
 */
class ShiftRightConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const ShiftRightConstraint c) {
        os << c.def << ": " << c.op1 << " >> " << c.op2;
        return os;
    }
};

/**
 * @class BitwiseAndConstraint
 * @brief Models abstract bitwise AND: v0 = v1 & v2
 */
class BitwiseAndConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const BitwiseAndConstraint c) {
        os << c.def << ": " << c.op1 << " & " << c.op2;
        return os;
    }
};

/**
 * @class BitwiseXorConstraint
 * @brief Models abstract bitwise XOR: v0 = v1 ^ v2
 */
class BitwiseXorConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const BitwiseXorConstraint c) {
        os << c.def << ": " << c.op1 << " ^ " << c.op2;
        return os;
    }
};

/**
 * @class BitwiseOrConstraint
 * @brief Models abstract bitwise OR: v0 = v1 | v2
 */
class BitwiseOrConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const BitwiseOrConstraint c) {
        os << c.def << ": " << c.op1 << " | " << c.op2;
        return os;
    }
};

/**
 * @class NegConstraint
 * @brief Models unary negation: v0 = -v1
 */
class NegConstraint : public UnaryConstraint {
public:
    using UnaryConstraint::UnaryConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const NegConstraint c) {
        os << c.def << ": -" << c.operand;
        return os;
    }
};

/**
 * @class IncConstraint
 * @brief Models increment: v0 = v1 + 1
 */
class IncConstraint : public UnaryConstraint {
public:
    using UnaryConstraint::UnaryConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const IncConstraint c) {
        os << c.def << ": " << c.operand << "++";
        return os;
    }
};

/**
 * @class DecConstraint
 * @brief Models decrement: v0 = v1 - 1
 */
class DecConstraint : public UnaryConstraint {
public:
    using UnaryConstraint::UnaryConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const DecConstraint c) {
        os << c.def << ": " << c.operand << "--";
        return os;
    }
};

/**
 * @class BitwiseNotConstraint
 * @brief Models bitwise complement: v0 = ~v1
 */
class BitwiseNotConstraint : public UnaryConstraint {
public:
    using UnaryConstraint::UnaryConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const BitwiseNotConstraint c) {
        os << c.def << ": ~" << c.operand;
        return os;
    }
};

/**
 * @class MultiplyConstraint
 * @brief Models abstract multiplication: v0 = v1 * v2
 */
class MultiplyConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const MultiplyConstraint c) {
        os << c.def << ": " << c.op1 << " * " << c.op2;
        return os;
    }
};

/**
 * @class InitializationConstraint
 * @brief Models literal assignments: v = c
 */
class InitializationBoolConstraint : public Constraint {
private:
    bool constant;
public:
    InitializationBoolConstraint(std::string var, bool c);
    bool eval(AbstractState& A) override;

    std::vector<UseEdge> get_uses() const override {
        std::vector<UseEdge> ret = {};
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const InitializationBoolConstraint& c) {
        os << c.def << ": " << c.constant;
        return os;
    }
};

/**
 * @class InitializationConstraint
 * @brief Models literal assignments: v = c
 */
class InitializationBoolTop : public Constraint {
public:
    InitializationBoolTop(std::string var);
    bool eval(AbstractState& A) override;

    std::vector<UseEdge> get_uses() const override {
        std::vector<UseEdge> ret = {};
        return ret;
    }

    friend std::ostream& operator<<(std::ostream& os, const InitializationBoolTop& c) {
        os << c.def << ": [false, true]";
        return os;
    }
};

/**
 * @class EqualConstraint
 * @brief Models v0 = (v1 == v2)
 */
class EqualConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const EqualConstraint c) {
        os << c.def << ": " << c.op1 << " == " << c.op2;
        return os;
    }
};

/**
 * @class NotEqualConstraint
 * @brief Models v0 = (v1 != v2)
 */
class NotEqualConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const NotEqualConstraint c) {
        os << c.def << ": " << c.op1 << " != " << c.op2;
        return os;
    }
};

/**
 * @class LessThanConstraint
 * @brief Models v0 = (v1 < v2)
 */
class LessThanConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const LessThanConstraint c) {
        os << c.def << ": " << c.op1 << " < " << c.op2;
        return os;
    }
};

/**
 * @class GreaterThanConstraint
 * @brief Models v0 = (v1 > v2)
 */
class GreaterThanConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const GreaterThanConstraint c) {
        os << c.def << ": " << c.op1 << " > " << c.op2;
        return os;
    }
};

/**
 * @class LessEqualConstraint
 * @brief Models v0 = (v1 <= v2)
 */
class LessEqualConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const LessEqualConstraint c) {
        os << c.def << ": " << c.op1 << " <= " << c.op2;
        return os;
    }
};

/**
 * @class GreaterEqualConstraint
 * @brief Models v0 = (v1 >= v2)
 */
class GreaterEqualConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const GreaterEqualConstraint c) {
        os << c.def << ": " << c.op1 << " >= " << c.op2;
        return os;
    }
};

/**
 * @class LogicalAndConstraint
 * @brief Models v0 = (v1 && v2).
 */
class LogicalAndConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const LogicalAndConstraint c) {
        os << c.def << ": " << c.op1 << " && " << c.op2;
        return os;
    }
};

/**
 * @class LogicalOrConstraint
 * @brief Models v0 = (v1 || v2).
 */
class LogicalOrConstraint : public ArithmeticConstraint {
public:
    using ArithmeticConstraint::ArithmeticConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const LogicalOrConstraint c) {
        os << c.def << ": " << c.op1 << " || " << c.op2;
        return os;
    }
};

/**
 * @class LogicalNotConstraint
 * @brief Models v0 = !v1, treating BoolValue's "true"/"false" the usual way.
 */
class LogicalNotConstraint : public UnaryConstraint {
public:
    using UnaryConstraint::UnaryConstraint;
    bool eval(AbstractState& A) override;

    friend std::ostream& operator<<(std::ostream& os, const LogicalNotConstraint c) {
        os << c.def << ": !" << c.operand;
        return os;
    }
};

inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<Constraint>& c) {
    if (auto initc = std::dynamic_pointer_cast<InitializationConstraint>(c)) {
        os << *initc;
    } else if (auto phic = std::dynamic_pointer_cast<PhiConstraint>(c)) {
        os << *phic;
    } else if (auto addc = std::dynamic_pointer_cast<AddConstraint>(c)) {
        os << *addc;
    } else if (auto interc = std::dynamic_pointer_cast<IntersectionConstraint>(c)) {
        os << *interc;
    } else if (auto mulc = std::dynamic_pointer_cast<MultiplyConstraint>(c)) {
        os << *mulc;
    } else if (auto subc = std::dynamic_pointer_cast<SubConstraint>(c)) {
        os << *subc;
    } else if (auto divc = std::dynamic_pointer_cast<DivConstraint>(c)) {
        os << *divc;
    } else if (auto modc = std::dynamic_pointer_cast<ModConstraint>(c)) {
        os << *modc;
    } else if (auto shiftlc = std::dynamic_pointer_cast<ShiftLeftConstraint>(c)) {
        os << *shiftlc;
    } else if (auto shiftrc = std::dynamic_pointer_cast<ShiftRightConstraint>(c)) {
        os << *shiftrc;
    } else if (auto bitandc = std::dynamic_pointer_cast<BitwiseAndConstraint>(c)) {
        os << *bitandc;
    } else if (auto bitxorc = std::dynamic_pointer_cast<BitwiseXorConstraint>(c)) {
        os << *bitxorc;
    } else if (auto bitorc = std::dynamic_pointer_cast<BitwiseOrConstraint>(c)) {
        os << *bitorc;
    } else if (auto negc = std::dynamic_pointer_cast<NegConstraint>(c)) {
        os << *negc;
    } else if (auto incc = std::dynamic_pointer_cast<IncConstraint>(c)) {
        os << *incc;
    } else if (auto decc = std::dynamic_pointer_cast<DecConstraint>(c)) {
        os << *decc;
    } else if (auto bitnotc = std::dynamic_pointer_cast<BitwiseNotConstraint>(c)) {
        os << *bitnotc;
    } else if (auto eqc = std::dynamic_pointer_cast<EqualConstraint>(c)) {
        os << *eqc;
    } else if (auto nec = std::dynamic_pointer_cast<NotEqualConstraint>(c)) {
        os << *nec;
    } else if (auto ltc = std::dynamic_pointer_cast<LessThanConstraint>(c)) {
        os << *ltc;
    } else if (auto gtc = std::dynamic_pointer_cast<GreaterThanConstraint>(c)) {
        os << *gtc;
    } else if (auto lec = std::dynamic_pointer_cast<LessEqualConstraint>(c)) {
        os << *lec;
    } else if (auto gec = std::dynamic_pointer_cast<GreaterEqualConstraint>(c)) {
        os << *gec;
    } else if (auto lac = std::dynamic_pointer_cast<LogicalAndConstraint>(c)) {
        os << *lac;
    } else if (auto loc = std::dynamic_pointer_cast<LogicalOrConstraint>(c)) {
        os << *loc;
    } else if (auto lnc = std::dynamic_pointer_cast<LogicalNotConstraint>(c)) {
        os << *lnc;
    } else if (auto rc = std::dynamic_pointer_cast<RangeConstraint>(c)) {
        os << *rc;
    } else {
        os << *c.get();
    }
    return os;
}
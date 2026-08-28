/**
 * @file Constraint.cpp
 * @brief Concrete implementations of constraint evaluation routines.
 */

#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"
#include "cangjie/Competition/RangeAnalysisSolver/AbstractValue.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <iostream>
#include <numeric>
#include <cstdlib>

namespace {

// --- Small shared helpers for the new IV-typed constraints below --------
// These mirror the getLower/getUpper-style lambdas already used inline in
// AddConstraint::eval and MultiplyConstraint::eval, factored out so the new
// binary/unary constraints don't each redefine them.

Bound valueLower(const IV &v) {
  if (v.getKind() == IV::Kind::Set) {
    // Caller is expected to have already handled the empty (bottom) case.
    return Bound::constant(*v.getValues().begin());
  }
  return v.getLower();
}

Bound valueUpper(const IV &v) {
  if (v.getKind() == IV::Kind::Set) {
    return Bound::constant(*v.getValues().rbegin());
  }
  return v.getUpper();
}

unsigned valueStride(const IV &v) {
  return (v.getKind() == IV::Kind::StridedInterval) ? v.getStride() : 1;
}

// Smallest value of the form 2^k - 1 that is >= maxVal (or 0 if maxVal <= 0).
// Used as a sound (but not tight) upper bound for OR/XOR of two
// non-negative operands: neither operation can set a bit beyond the
// highest bit present in either operand.
int upperBoundForOrXor(int maxVal) {
  if (maxVal <= 0) return 0;
  unsigned mask = 1;
  while (mask < static_cast<unsigned>(maxVal)) mask = (mask << 1) | 1u;
  return static_cast<int>(mask);
}

} // namespace

// --- Base Constraint ---
Constraint::Constraint(std::string name) : def(std::move(name)) {}

// --- InitializationConstraint (v = c) ---
InitializationConstraint::InitializationConstraint(std::string var, int c)
    : Constraint(std::move(var)), constant(c) {}

bool InitializationConstraint::eval(AbstractState& A) {
    IV old_val = std::get<IV>(A[def]);

    std::vector<int> vals = {constant};
    std::get<IV>(A[def]).addConstant(vals);
    return old_val != std::get<IV>(A[def]); // Leverages your custom equality operator!
}

// --- InitializationConstraint (v = c) ---
InitializationIntegerTop::InitializationIntegerTop(std::string var)
    : Constraint(std::move(var)) {}

bool InitializationIntegerTop::eval(AbstractState& A) {
    IV old_val = std::get<IV>(A[def]);

    std::get<IV>(A[def]).setAsTop();
    return old_val != std::get<IV>(A[def]); // Leverages your custom equality operator!
}

// --- PhiConstraint (v0 = phi(v1, v2, ...)) ---
PhiConstraint::PhiConstraint(std::string var, std::vector<std::string> ops,
    ValueType type)
    : Constraint(std::move(var)), operands(std::move(ops)), type(type) {}

bool PhiConstraint::eval(AbstractState& A) {
    if (!A.count(def)) {
        // A PHI constraint binds a series of OTHER variables with same type to
        // `def`. There is a possibility that NONE of the phi ops were defined
        // by a constraint, so we have to pass in the `type` to the constraint.
        if (type == ValueType::IVType) {
            A.try_emplace(def, IV());
        } else {
            A.try_emplace(def, BV());
        }
    }
    if (std::holds_alternative<BV>(A[def])) {
        BV old_val = std::get<BV>(A[def]);
        BV accumulated_join; // Starts at bottom element
    
        for (const auto &op : operands) {
            // ! Error here in i-plus-j code: std::bad_variant_access
            accumulated_join.join(std::get<BV>(A[op]));
        }
    
        std::get<BV>(A[def]) = accumulated_join;
        return old_val != accumulated_join;
    } else {
        IV old_val = std::get<IV>(A[def]);
        IV accumulated_join; // Starts at bottom element
    
        for (const auto &op : operands) {
          accumulated_join.join(std::get<IV>(A[op]));
        }
    
        std::get<IV>(A[def]) = accumulated_join;
        return old_val != accumulated_join;
    }
}

// --- IntersectionConstraint (v0 = v1 intersection [low, up]) ---
IntersectionConstraint::IntersectionConstraint(std::string dest,
                                               std::string src,
                                               IntersectionBound low,
                                               IntersectionBound up)
    : Constraint(std::move(dest)), operand(std::move(src)),
      lower_bound(std::move(low)), upper_bound(std::move(up)) {}

Bound
IntersectionConstraint::resolveBound(const IntersectionBound &b,
                                     const bool isLower,
                                     const AbstractState &) const {

  if (std::holds_alternative<Bound>(b))
    return std::get<Bound>(b);

  // Growth phase:
  // ignore symbolic references
  Bound result;

  if (isLower)
    result = Bound::minusInfinity();
  else
    result = Bound::plusInfinity();

  return result;
}

IntersectionConstraint
IntersectionConstraint::resolveFutures(const AbstractState &state) const {
    auto resolve = [&](const IntersectionBound &bound,
      bool isLower) -> IntersectionBound {
    if (std::holds_alternative<Bound>(bound))
      return bound;

    const Future &future = std::get<Future>(bound);

    auto it = state.find(future.target_variable);
    assert(it != state.end());

    IV futureVariable = std::get<IV>(it->second);

    Bound result =
      isLower ? futureVariable.getLower()
      : futureVariable.getUpper();

    if (result.isConstant())
      result.setConstant(result.getConstant() + future.offset);

    return result;
  };

  return IntersectionConstraint(
      def,
      operand,
      resolve(lower_bound, true),
      resolve(upper_bound, false));
}

bool IntersectionConstraint::eval(AbstractState &A) {
    IV oldValue = std::get<IV>(A[def]);
    IV src = std::get<IV>(A[operand]);

  auto low = resolveBound(lower_bound, true, A);
  auto up = resolveBound(upper_bound, false, A);

  // Bottom operand turns into [-inf, +inf]
  if (src.getKind() == IV::Kind::Set && src.getValues().empty()) {
    src.setAsTop();
  }

  // Source is a finite set.
  if (src.getKind() == IV::Kind::Set) {

    std::vector<int> vals;
    for (int v : src.getValues()) {
      bool keep = true;
      if (low.isConstant())
        keep &= (v >= low.getConstant());
      if (up.isConstant())
        keep &= (v <= up.getConstant());
      if (keep)
        vals.emplace_back(v);
    }

    if (!vals.empty())
      std::get<IV>(A[def]).addConstant(vals);
  }

  // Source is already an interval.
  else {
    auto lower = src.getLower();
    auto upper = src.getUpper();

    // max(lower, low)
    if (low.isConstant()) {
      if (lower.isMinusInfinity())
        lower = low;
      else if (lower.isConstant())
        lower.setConstant(std::max(lower.getConstant(), low.getConstant()));
    }

    // min(upper, up)
    if (up.isConstant()) {
      if (upper.isPlusInfinity())
        upper = up;
      else if (upper.isConstant())
        upper.setConstant(std::min(upper.getConstant(), up.getConstant()));
    }

    // Empty interval?
    if (lower.isConstant() &&
        upper.isConstant() &&
        lower.getConstant() > upper.getConstant()) {
      // Leave result as bottom.
    } else {
      std::get<IV>(A[def]).setAsInterval(lower, upper, src.getStride());
    }
  }

    return oldValue != std::get<IV>(A[def]);
}

// --- UnaryConstraint Base ---
UnaryConstraint::UnaryConstraint(std::string dest, std::string src)
    : Constraint(std::move(dest)), operand(std::move(src)) {}

// --- ArithmeticConstraint Base ---
ArithmeticConstraint::ArithmeticConstraint(std::string dest, std::string lhs,
                                           std::string rhs)
    : Constraint(std::move(dest)), op1(std::move(lhs)), op2(std::move(rhs)) {}

bool AddConstraint::eval(AbstractState& A) {
    IV old_val = std::get<IV>(A[def]);
    
    const IV &lhs = std::get<IV>(A[op1]);
    const IV &rhs = std::get<IV>(A[op2]);
    
    // AnalyzedValue result; // Starts at bottom (empty set)

    // Exact evaluation: finite set × finite set
    if (lhs.getKind() == IV::Kind::Set &&
        rhs.getKind() == IV::Kind::Set) {

        // Bottom propagates.
        if (lhs.getValues().empty() || rhs.getValues().empty()) {
            std::get<IV>(A[def]).setAsBottom();
            return old_val != std::get<IV>(A[def]);
        }

        std::vector<int> consts;

        for (int l : lhs.getValues()) {
            for (int r : rhs.getValues()) {
                consts.emplace_back(l + r);
            }
        }

        std::get<IV>(A[def]).addConstant(consts);

    return old_val != std::get<IV>(A[def]);
  }

  // Otherwise treat every operand as a strided interval.
  auto getLower = [](const IV &v) -> Bound {
    if (v.getKind() == IV::Kind::Set) {
      return Bound::constant(*v.getValues().begin());
    }
    return v.getLower();
  };

  auto getUpper = [](const IV &v) -> Bound {
    if (v.getKind() == IV::Kind::Set) {
      if (v.getValues().empty())
        return Bound::constant(*v.getValues().begin());
              
      return Bound::constant(*v.getValues().rbegin());
    }
    return v.getUpper();
  };

  auto addLower =
      [](const Bound &a,
         const Bound &b) -> Bound {

    if (a.isMinusInfinity() ||
        b.isMinusInfinity())
      return Bound::minusInfinity();

    return Bound::constant(a.getConstant() + b.getConstant());
  };

  auto addUpper =
      [](const Bound &a,
         const Bound &b) -> Bound {

    if (a.isPlusInfinity() ||
        b.isPlusInfinity())
      return Bound::plusInfinity();

    return Bound::constant(a.getConstant() + b.getConstant());
  };

  auto lower = addLower(getLower(lhs), getLower(rhs));
  auto upper = addUpper(getUpper(lhs), getUpper(rhs));

  unsigned s1 =
      (lhs.getKind() == IV::Kind::StridedInterval)
          ? lhs.getStride()
          : 1;

  unsigned s2 =
      (rhs.getKind() == IV::Kind::StridedInterval)
          ? rhs.getStride()
          : 1;

  std::get<IV>(A[def]).setAsInterval(lower, upper, std::gcd(s1, s2));

  return old_val != std::get<IV>(A[def]);
}

bool SubConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  // Exact evaluation: finite set x finite set.
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        consts.emplace_back(l - r);

    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  // Otherwise treat every operand as a strided interval. For a - b, the
  // result range is [lhs.lower - rhs.upper, lhs.upper - rhs.lower].
  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  auto subLower = [](const Bound &a, const Bound &b) -> Bound {
    if (a.isMinusInfinity() || b.isPlusInfinity())
      return Bound::minusInfinity();
    return Bound::constant(a.getConstant() - b.getConstant());
  };

  auto subUpper = [](const Bound &a, const Bound &b) -> Bound {
    if (a.isPlusInfinity() || b.isMinusInfinity())
      return Bound::plusInfinity();
    return Bound::constant(a.getConstant() - b.getConstant());
  };

  Bound lower = subLower(lLower, rUpper);
  Bound upper = subUpper(lUpper, rLower);

  std::get<IV>(A[def]).setAsInterval(
      lower, upper, std::gcd(valueStride(lhs), valueStride(rhs)));
  return old_val != std::get<IV>(A[def]);
}

// --- DivConstraint (v0 = v1 / v2) ---
bool DivConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        if (r != 0) // division by zero is UB; that combination is unreachable
          consts.emplace_back(l / r);

    if (consts.empty()) {
      std::get<IV>(A[def]).setAsBottom(); // every divisor was zero
    } else {
      std::get<IV>(A[def]).addConstant(consts);
    }
    return old_val != std::get<IV>(A[def]);
  }

  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  // If the divisor's range may include zero, or any endpoint is unbounded,
  // dividing can produce arbitrarily large magnitudes, so fall back to the
  // unconstrained interval rather than something unsound.
  bool divisorMayBeZero =
      (!rLower.isConstant() || rLower.getConstant() <= 0) &&
      (!rUpper.isConstant() || rUpper.getConstant() >= 0);

  if (divisorMayBeZero || !lLower.isConstant() || !lUpper.isConstant() ||
      !rLower.isConstant() || !rUpper.isConstant()) {
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
    return old_val != std::get<IV>(A[def]);
  }

  // All four bounds are finite and the divisor's range excludes zero:
  // evaluate the quotient at each of the four corners to soundly bound it.
  int a = lLower.getConstant(), b = lUpper.getConstant();
  int c = rLower.getConstant(), d = rUpper.getConstant();
  int corners[4] = {a / c, a / d, b / c, b / d};
  int lo = *std::min_element(corners, corners + 4);
  int hi = *std::max_element(corners, corners + 4);
  std::get<IV>(A[def]).setAsInterval(Bound::constant(lo), Bound::constant(hi), 1);
  return old_val != std::get<IV>(A[def]);
}

// --- ModConstraint (v0 = v1 % v2) ---
bool ModConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        if (r != 0)
          consts.emplace_back(l % r);

    if (consts.empty()) {
      std::get<IV>(A[def]).setAsBottom();
    } else {
      std::get<IV>(A[def]).addConstant(consts);
    }
    return old_val != std::get<IV>(A[def]);
  }

  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  if (!rLower.isConstant() || !rUpper.isConstant()) {
    // Divisor magnitude is unbounded: the remainder's magnitude is
    // unbounded too.
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
    return old_val != std::get<IV>(A[def]);
  }

  // |a % b| < |b| for any nonzero b under C++'s truncating semantics, so
  // bound the result by the largest divisor magnitude in range.
  int m = std::max(std::abs(rLower.getConstant()), std::abs(rUpper.getConstant()));
  if (m == 0) {
    std::get<IV>(A[def]).setAsBottom(); // the divisor's range is exactly {0}
    return old_val != std::get<IV>(A[def]);
  }
  std::get<IV>(A[def]).setAsInterval(Bound::constant(-(m - 1)), Bound::constant(m - 1), 1);
  return old_val != std::get<IV>(A[def]);
}

// --- ShiftLeftConstraint (v0 = v1 << v2) ---
bool ShiftLeftConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        if (r >= 0) // negative shift amounts are UB
          consts.emplace_back(l << r);

    if (consts.empty()) {
      std::get<IV>(A[def]).setAsBottom();
    } else {
      std::get<IV>(A[def]).addConstant(consts);
    }
    return old_val != std::get<IV>(A[def]);
  }

  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  // Left-shifting a negative value is undefined behavior, and unbounded
  // endpoints make a precise result impossible, so only attempt a tight
  // bound when everything is finite and non-negative.
  if (lLower.isConstant() && lUpper.isConstant() && rLower.isConstant() &&
      rUpper.isConstant() && lLower.getConstant() >= 0 &&
      rLower.getConstant() >= 0) {
    int a = lLower.getConstant(), b = lUpper.getConstant();
    int c = rLower.getConstant(), d = rUpper.getConstant();
    int corners[4] = {a << c, a << d, b << c, b << d};
    int lo = *std::min_element(corners, corners + 4);
    int hi = *std::max_element(corners, corners + 4);
    std::get<IV>(A[def]).setAsInterval(Bound::constant(lo), Bound::constant(hi), 1);
  } else {
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
  }
  return old_val != std::get<IV>(A[def]);
}

// --- ShiftRightConstraint (v0 = v1 >> v2) ---
bool ShiftRightConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        if (r >= 0)
          consts.emplace_back(l >> r);

    if (consts.empty()) {
      std::get<IV>(A[def]).setAsBottom();
    } else {
      std::get<IV>(A[def]).addConstant(consts);
    }
    return old_val != std::get<IV>(A[def]);
  }

  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  // Arithmetic right shift is monotonic in the shifted value for a fixed,
  // non-negative shift amount, so corner evaluation is sound here even
  // when the shifted value is negative.
  if (lLower.isConstant() && lUpper.isConstant() && rLower.isConstant() &&
      rUpper.isConstant() && rLower.getConstant() >= 0) {
    int a = lLower.getConstant(), b = lUpper.getConstant();
    int c = rLower.getConstant(), d = rUpper.getConstant();
    int corners[4] = {a >> c, a >> d, b >> c, b >> d};
    int lo = *std::min_element(corners, corners + 4);
    int hi = *std::max_element(corners, corners + 4);
    std::get<IV>(A[def]).setAsInterval(Bound::constant(lo), Bound::constant(hi), 1);
  } else {
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
  }
  return old_val != std::get<IV>(A[def]);
}

// --- BitwiseAndConstraint (v0 = v1 & v2) ---
bool BitwiseAndConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        consts.emplace_back(l & r);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  // Bitwise AND of two non-negative values is never negative and never
  // exceeds the smaller of the two upper bounds.
  if (lLower.isConstant() && lLower.getConstant() >= 0 &&
      rLower.isConstant() && rLower.getConstant() >= 0 &&
      lUpper.isConstant() && rUpper.isConstant()) {
    int hi = std::min(lUpper.getConstant(), rUpper.getConstant());
    std::get<IV>(A[def]).setAsInterval(Bound::constant(0), Bound::constant(hi), 1);
  } else {
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
  }
  return old_val != std::get<IV>(A[def]);
}

// --- BitwiseXorConstraint (v0 = v1 ^ v2) ---
bool BitwiseXorConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        consts.emplace_back(l ^ r);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  // For non-negative operands, XOR can't set a bit above the highest bit
  // present in either operand -- a sound, if not maximally tight, bound.
  if (lLower.isConstant() && lLower.getConstant() >= 0 &&
      rLower.isConstant() && rLower.getConstant() >= 0 &&
      lUpper.isConstant() && rUpper.isConstant()) {
    int hi = upperBoundForOrXor(std::max(lUpper.getConstant(), rUpper.getConstant()));
    std::get<IV>(A[def]).setAsInterval(Bound::constant(0), Bound::constant(hi), 1);
  } else {
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
  }
  return old_val != std::get<IV>(A[def]);
}

// --- BitwiseOrConstraint (v0 = v1 | v2) ---
bool BitwiseOrConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {

    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }

    std::vector<int> consts;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        consts.emplace_back(l | r);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  Bound lLower = valueLower(lhs), lUpper = valueUpper(lhs);
  Bound rLower = valueLower(rhs), rUpper = valueUpper(rhs);

  if (lLower.isConstant() && lLower.getConstant() >= 0 &&
      rLower.isConstant() && rLower.getConstant() >= 0 &&
      lUpper.isConstant() && rUpper.isConstant()) {
    int hi = upperBoundForOrXor(std::max(lUpper.getConstant(), rUpper.getConstant()));
    std::get<IV>(A[def]).setAsInterval(Bound::constant(0), Bound::constant(hi), 1);
  } else {
    std::get<IV>(A[def]).setAsInterval(Bound::minusInfinity(), Bound::plusInfinity(), 1);
  }
  return old_val != std::get<IV>(A[def]);
}

// --- NegConstraint (v0 = -v1) ---
bool NegConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);
  const IV &src = std::get<IV>(A[operand]);

  if (src.getKind() == IV::Kind::Set) {
    if (src.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }
    std::vector<int> consts;
    for (int v : src.getValues()) consts.emplace_back(-v);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  Bound lo = src.getLower(), hi = src.getUpper();
  Bound newLower = hi.isPlusInfinity() ? Bound::minusInfinity() : Bound::constant(-hi.getConstant());
  Bound newUpper = lo.isMinusInfinity() ? Bound::plusInfinity() : Bound::constant(-lo.getConstant());
  std::get<IV>(A[def]).setAsInterval(newLower, newUpper, src.getStride());
  return old_val != std::get<IV>(A[def]);
}

// --- IncConstraint (v0 = v1 + 1) ---
bool IncConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);
  const IV &src = std::get<IV>(A[operand]);

  if (src.getKind() == IV::Kind::Set) {
    if (src.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }
    std::vector<int> consts;
    for (int v : src.getValues()) consts.emplace_back(v + 1);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  Bound lo = src.getLower(), hi = src.getUpper();
  Bound newLower = lo.isMinusInfinity() ? lo : Bound::constant(lo.getConstant() + 1);
  Bound newUpper = hi.isPlusInfinity() ? hi : Bound::constant(hi.getConstant() + 1);
  std::get<IV>(A[def]).setAsInterval(newLower, newUpper, src.getStride());
  return old_val != std::get<IV>(A[def]);
}

// --- DecConstraint (v0 = v1 - 1) ---
bool DecConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);
  const IV &src = std::get<IV>(A[operand]);

  if (src.getKind() == IV::Kind::Set) {
    if (src.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }
    std::vector<int> consts;
    for (int v : src.getValues()) consts.emplace_back(v - 1);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  Bound lo = src.getLower(), hi = src.getUpper();
  Bound newLower = lo.isMinusInfinity() ? lo : Bound::constant(lo.getConstant() - 1);
  Bound newUpper = hi.isPlusInfinity() ? hi : Bound::constant(hi.getConstant() - 1);
  std::get<IV>(A[def]).setAsInterval(newLower, newUpper, src.getStride());
  return old_val != std::get<IV>(A[def]);
}

// --- BitwiseNotConstraint (v0 = ~v1) ---
bool BitwiseNotConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);
  const IV &src = std::get<IV>(A[operand]);

  if (src.getKind() == IV::Kind::Set) {
    if (src.getValues().empty()) {
      std::get<IV>(A[def]).setAsBottom();
      return old_val != std::get<IV>(A[def]);
    }
    std::vector<int> consts;
    for (int v : src.getValues()) consts.emplace_back(~v);
    std::get<IV>(A[def]).addConstant(consts);
    return old_val != std::get<IV>(A[def]);
  }

  // ~x == -x - 1, so the interval flips and shifts by one.
  Bound lo = src.getLower(), hi = src.getUpper();
  Bound newLower = hi.isPlusInfinity() ? Bound::minusInfinity() : Bound::constant(-(hi.getConstant()) - 1);
  Bound newUpper = lo.isMinusInfinity() ? Bound::plusInfinity() : Bound::constant(-(lo.getConstant()) - 1);
  std::get<IV>(A[def]).setAsInterval(newLower, newUpper, src.getStride());
  return old_val != std::get<IV>(A[def]);
}

bool MultiplyConstraint::eval(AbstractState &A)
{
  IV old = std::get<IV>(A[this->def]);

  IV lhs = std::get<IV>(A[this->op1]);
  IV rhs = std::get<IV>(A[this->op2]);

  IV result;

  if(lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set)
  {
    if (lhs.getValues().empty() || rhs.getValues().empty()) {
      std::get<IV>(A[def]) = result;
      return old != result;
    }

    std::vector<int> consts;
    for (int l : lhs.getValues()) {
        for (int r : rhs.getValues()) {
            consts.emplace_back(l * r);
        }
    }
    result.addConstant(consts);
  } else {

    // Otherwise treat every operand as a strided interval.
    auto getLower = [](const IV &v) -> Bound {
      if (v.getKind() == IV::Kind::Set) {
        return Bound::constant(*v.getValues().begin());
      }
      return v.getLower();
    };

    auto getUpper = [](const IV &v) -> Bound {
      if (v.getKind() == IV::Kind::Set) {
        if (v.getValues().empty())
          return Bound::constant(*v.getValues().begin());
                
        return Bound::constant(*v.getValues().rbegin());
      }
      return v.getUpper();
    };

    auto multiplyBounds = [](const Bound &x, const Bound &y) {
      Bound l = std::min(x,y);
      Bound h = std::max(x,y);

      if (l.isMinusInfinity()) {
        if (h.isMinusInfinity()) {        // -inf * -inf = +inf
          return Bound::plusInfinity();
        } else if (h.isConstant()) {
          if (h.getConstant() < 0)        // -inf * -C = +inf
            return Bound::plusInfinity();
          else if (h.getConstant() == 0)  // -inf * 0 = 0
            return Bound::constant(0);
          else                            // -inf * +C = -inf
            return Bound::minusInfinity();
        } else {                          // -inf * +inf = -inf
          return Bound::minusInfinity();
        }
      } else if (l.isConstant()) {
        if (h.isConstant()) {             // C * C
          return Bound::constant(
            l.getConstant() * h.getConstant()
          );
        } else {
          if (l.getConstant() < 0)        // -C * +inf = -inf
            return Bound::minusInfinity();
          else if (l.getConstant() == 0)  // 0 * +inf = 0
            return Bound::constant(0);
          else                            // C * +inf = +inf
            return Bound::plusInfinity();
        }
      } else {                            // +inf * +inf = +inf
        return Bound::plusInfinity();
      }
    };

    Bound l1 = getLower(lhs);
    Bound l2 = getLower(rhs);
    Bound u1 = getUpper(lhs);
    Bound u2 = getUpper(rhs);

    Bound p1 = multiplyBounds(l1,l2);
    Bound p2 = multiplyBounds(l1,u2);
    Bound p3 = multiplyBounds(u1,l2);
    Bound p4 = multiplyBounds(u1,u2);

    Bound min = std::min({p1, p2, p3, p4});
    Bound max = std::max({p1, p2, p3, p4});

    result.setAsInterval(min,max,1); // FIXME: Interval stride
}

  A[this->def] = result;
  return old != result;
}

// --- InitializationConstraint (v = c) ---
InitializationBoolConstraint::InitializationBoolConstraint(std::string var, bool c)
    : Constraint(std::move(var)), constant(c) {}

bool InitializationBoolConstraint::eval(AbstractState& A) {
    // Emplace a BoolValue if def is undefined in the Abstract State
    A.try_emplace(def, BV());
    BV old_val = std::get<BV>(A[def]);

    std::vector<bool> vals = {constant};
    std::get<BV>(A[def]).addConstant(vals);
    return old_val != std::get<BV>(A[def]); // Leverages your custom equality operator!
}

// --- InitializationConstraint (v = c) ---
InitializationBoolTop::InitializationBoolTop(std::string var)
    : Constraint(std::move(var)) {}

bool InitializationBoolTop::eval(AbstractState& A) {
    // Emplace a BoolValue if def is undefined in the Abstract State
    A.try_emplace(def, BV());
    BV old_val = std::get<BV>(A[def]);
    std::get<BV>(A[def]).setAsTop();

    return old_val != std::get<BV>(A[def]); // Leverages your custom equality operator!
}

// --- EqualConstraint (v0 = v1 == v2) ---
bool EqualConstraint::eval(AbstractState& A) {
  // Emplace a BoolValue if def is undefined in the Abstract State
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);


  std::vector<bool> vals;
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {
    std::set<int> aux;

    // If the intersection is not empty, then v1 == v2 can be true
    std::set_intersection(lhs.getValues().begin(), lhs.getValues().end(),
                  rhs.getValues().begin(), rhs.getValues().end(),
                  std::inserter(aux, aux.begin()));

    if (!aux.empty()) vals.push_back(true);

    aux.clear();
    // If the difference is not empty, then v1 == v2 can be false
    std::set_difference(lhs.getValues().begin(), lhs.getValues().end(),
                  rhs.getValues().begin(), rhs.getValues().end(),
                  std::inserter(aux, aux.begin()));
    
    if (!aux.empty()) vals.push_back(false);
  } else if (lhs == rhs) {
    vals.push_back(true);
    if (lhs.getLower() != lhs.getUpper())
      vals.push_back(false);
  } else if (lhs < rhs || lhs > rhs) {
    vals.push_back(false);
  } else {
    vals.push_back(true);
    vals.push_back(false);
  }
  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- LogicalAndConstraint (v0 = v1 && v2) ---
bool LogicalAndConstraint::eval(AbstractState& A) {
  // Emplace a BoolValue if def is undefined in the Abstract State
  A.try_emplace(def, BV());
  
  BV old_val = std::get<BV>(A[def]);
  const BV &lhs = std::get<BV>(A[op1]);
  const BV &rhs = std::get<BV>(A[op2]);

  std::vector<bool> vals;

  // If v1 or v2 can be false, then v0 can be false
  if (lhs.getValues().count(false) || rhs.getValues().count(false))
    vals.push_back(false);
  
  // If v1 and v2 can be true, then v0 can be true
  if (lhs.getValues().count(true) && rhs.getValues().count(true))
    vals.push_back(true);

  std::get<BV>(A[def]).addConstant(vals);

  return old_val != std::get<BV>(A[def]);
}

// --- NotEqualConstraint (v0 = v1 != v2) ---
// Mirrors EqualConstraint's set_intersection/set_difference idiom, with
// true/false swapped: an intersection means v1 == v2 is possible (so != can
// be false), and a difference means some value in v1 has no match in v2 (so
// != can be true).
bool NotEqualConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  std::vector<bool> vals;
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {
    std::set<int> aux;

    std::set_intersection(lhs.getValues().begin(), lhs.getValues().end(),
                  rhs.getValues().begin(), rhs.getValues().end(),
                  std::inserter(aux, aux.begin()));
    if (!aux.empty()) vals.push_back(false);

    aux.clear();
    std::set_difference(lhs.getValues().begin(), lhs.getValues().end(),
                  rhs.getValues().begin(), rhs.getValues().end(),
                  std::inserter(aux, aux.begin()));
    if (!aux.empty()) vals.push_back(true);
  } else if (lhs == rhs) {
    vals.push_back(false);
    if (lhs.getLower() != lhs.getUpper())
      vals.push_back(true);
  } else if (lhs < rhs || lhs > rhs) {
    vals.push_back(true);
  } else {
    vals.push_back(true);
    vals.push_back(false);
  }
  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- LessThanConstraint (v0 = v1 < v2) ---
// Unlike equality, "<" doesn't map cleanly onto set intersection/difference,
// so finite sets are evaluated exactly by checking every (l, r) pair (the
// same "cross every pair" technique AddConstraint/MultiplyConstraint use for
// arithmetic), and the interval case reuses AbstractValue's existing
// definitely-ordered comparison operators.
bool LessThanConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  std::vector<bool> vals;
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {
    bool anyTrue = false, anyFalse = false;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        (l < r ? anyTrue : anyFalse) = true;
    if (anyTrue) vals.push_back(true);
    if (anyFalse) vals.push_back(false);
  } else if (lhs < rhs) {
    vals.push_back(true);
  } else if (lhs >= rhs) {
    vals.push_back(false);
  } else {
    vals.push_back(true);
    vals.push_back(false);
  }
  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- GreaterThanConstraint (v0 = v1 > v2) ---
bool GreaterThanConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  std::vector<bool> vals;
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {
    bool anyTrue = false, anyFalse = false;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        (l > r ? anyTrue : anyFalse) = true;
    if (anyTrue) vals.push_back(true);
    if (anyFalse) vals.push_back(false);
  } else if (lhs > rhs) {
    vals.push_back(true);
  } else if (lhs <= rhs) {
    vals.push_back(false);
  } else {
    vals.push_back(true);
    vals.push_back(false);
  }
  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- LessEqualConstraint (v0 = v1 <= v2) ---
bool LessEqualConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  std::vector<bool> vals;
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {
    bool anyTrue = false, anyFalse = false;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        (l <= r ? anyTrue : anyFalse) = true;
    if (anyTrue) vals.push_back(true);
    if (anyFalse) vals.push_back(false);
  } else if (lhs <= rhs) {
    vals.push_back(true);
  } else if (lhs > rhs) {
    vals.push_back(false);
  } else {
    vals.push_back(true);
    vals.push_back(false);
  }
  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- GreaterEqualConstraint (v0 = v1 >= v2) ---
bool GreaterEqualConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const IV &lhs = std::get<IV>(A[op1]);
  const IV &rhs = std::get<IV>(A[op2]);

  std::vector<bool> vals;
  if (lhs.getKind() == IV::Kind::Set && rhs.getKind() == IV::Kind::Set) {
    bool anyTrue = false, anyFalse = false;
    for (int l : lhs.getValues())
      for (int r : rhs.getValues())
        (l >= r ? anyTrue : anyFalse) = true;
    if (anyTrue) vals.push_back(true);
    if (anyFalse) vals.push_back(false);
  } else if (lhs >= rhs) {
    vals.push_back(true);
  } else if (lhs < rhs) {
    vals.push_back(false);
  } else {
    vals.push_back(true);
    vals.push_back(false);
  }
  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- LogicalOrConstraint (v0 = v1 || v2) ---
bool LogicalOrConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const BV &lhs = std::get<BV>(A[op1]);
  const BV &rhs = std::get<BV>(A[op2]);

  std::vector<bool> vals;

  // If v1 or v2 can be true, then v0 can be true
  if (lhs.getValues().count(true) || rhs.getValues().count(true))
    vals.push_back(true);

  // If v1 and v2 can be false, then v0 can be false
  if (lhs.getValues().count(false) && rhs.getValues().count(false))
    vals.push_back(false);

  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- LogicalNotConstraint (v0 = !v1) ---
bool LogicalNotConstraint::eval(AbstractState& A) {
  A.try_emplace(def, BV());

  BV old_val = std::get<BV>(A[def]);
  const BV &src = std::get<BV>(A[operand]);

  std::vector<bool> vals;
  if (src.getValues().count(true)) vals.push_back(false);
  if (src.getValues().count(false)) vals.push_back(true);

  std::get<BV>(A[def]).addConstant(vals);
  return old_val != std::get<BV>(A[def]);
}

// --- RangeConstraint (v0 = low..high | low..=high [: step]) ---
Bound RangeConstraint::resolveBound(const RangeBound &b, bool isLower,
                                    const AbstractState &A) const {
  if (std::holds_alternative<Bound>(b))
    return std::get<Bound>(b);

  const Future &future = std::get<Future>(b);
  auto it = A.find(future.target_variable);
  if (it == A.end())
    return isLower ? Bound::minusInfinity() : Bound::plusInfinity();

  const IV &targetVal = std::get<IV>(it->second);
  Bound result = isLower ? targetVal.getLower() : targetVal.getUpper();
  if (result.isConstant())
    result.setConstant(result.getConstant() + future.offset);
  return result;
}

bool RangeConstraint::eval(AbstractState& A) {
  IV old_val = std::get<IV>(A[def]);

  Bound low = resolveBound(low_bound, true, A);
  Bound high = resolveBound(high_bound, false, A);

  // Right-open ranges (low..high) exclude the upper endpoint; shift it back
  // by one so it can be stored as an inclusive interval, like ..= .
  if (!inclusive && high.isConstant())
    high.setConstant(high.getConstant() - 1);

  // Empty range, e.g. 5..5 or 5..3.
  if (low.isConstant() && high.isConstant() && low.getConstant() > high.getConstant()) {
    std::get<IV>(A[def]).setAsBottom();
    return old_val != std::get<IV>(A[def]);
  }

  std::get<IV>(A[def]).setAsInterval(low, high, std::max(1u, step));
  return old_val != std::get<IV>(A[def]);
}
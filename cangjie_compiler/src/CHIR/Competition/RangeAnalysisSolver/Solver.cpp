/**
 * @file Solver.cpp
 * @brief Definition of the Solver class member functions.
 */

#include "cangjie/Competition/RangeAnalysisSolver/Solver.h"

// Define this to show all steps the solver takes
#define DEBUG_SHOW_SOLVER_STEPS

Solver::Solver(AbstractState& state) : state(state) {}

void Solver::addConstraint(std::shared_ptr<Constraint> constraint) {
    constraints.push_back(constraint);
}

void Solver::clear() { constraints.clear(); }

void Solver::resolveSCC() {
#ifdef DEBUG_SHOW_SOLVER_STEPS
    std::cout << "\nSolver called for the following constraints:\n";
    for (auto& constraint : this->constraints) {
        std::cout << "\t" << constraint << "\n";
    }

    std::cout << "Solver state:\n";
    for (auto [var, val] : state) {
        std::cout << "\t" << var << " = ";
        if (std::holds_alternative<BV>(val)) {
            std::cout << std::get<BV>(val);
        } else {
            std::cout << std::get<IV>(val);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
#endif

    growthAnalysis();

#ifdef DEBUG_SHOW_SOLVER_STEPS
    std::cout << "\nState after growth analysis:\n";
    for (auto const& [var, val] : state) {
        std::cout << "\t" << var << " = ";
        if (std::holds_alternative<BV>(val)) {
            std::cout << std::get<BV>(val);
        } else {
            std::cout << std::get<IV>(val);
        }
        std::cout << "\n";
    }

    // Make counters useless after growth analysis
    for (auto const& [var, val] : state) {
        if (std::holds_alternative<BV>(val)) {
            std::get<BV>(state[var]).resetCounter();
        } else {
            std::get<IV>(state[var]).resetCounter();
        }
    }
#endif

    futureResolution();

#ifdef DEBUG_SHOW_SOLVER_STEPS
    std::cout << "\nState after future resolution\n";
    for (auto const& [var, val] : state) {
        std::cout << "\t" << var << " = ";
        if (std::holds_alternative<BV>(val)) {
            std::cout << std::get<BV>(val);
        } else {
            std::cout << std::get<IV>(val);
        }
        std::cout << "\n";
    }
#endif

    narrowingAnalysis();

#ifdef DEBUG_SHOW_SOLVER_STEPS
    std::cout << "\nState after narrowing analysis\n";
    for (auto const& [var, val] : state) {
        std::cout << "\t" << var << " = ";
        if (std::holds_alternative<BV>(val)) {
            std::cout << std::get<BV>(val);
        } else {
            std::cout << std::get<IV>(val);
        }
        std::cout << "\n";
    }
#endif

    clear();
}

void Solver::solve(
    std::vector<std::vector<std::shared_ptr<Constraint>>>& sccs) {
    for (auto& scc : sccs) {
        for (auto& constraint : scc) {
            addConstraint(constraint);
        }
        resolveSCC();
    }
}

void Solver::growthAnalysis() {
    bool changed_evaluating = true;
    int iteration = 0;

    while (changed_evaluating) {
        changed_evaluating = false;
        ++iteration;

        for (auto& constraint : constraints) {
            if (constraint->eval(this->state)) {
                changed_evaluating = true;
            }
        }
    }
}

void Solver::narrowingAnalysis() {
    bool changed_narrowing = true;

    while (changed_narrowing) {
        changed_narrowing = false;

        for (auto& constraint : constraints) {
            if (constraint->narrow(this->state)) {
                changed_narrowing = true;
            }
        }
    }
}

void Solver::futureResolution() {
    for (auto& constraint : constraints) {
        auto intersection =
            std::dynamic_pointer_cast<IntersectionConstraint>(constraint);

        if (!intersection) continue;

        constraint = std::make_shared<IntersectionConstraint>(
            intersection->resolveFutures(state));
    }
}

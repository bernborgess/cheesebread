#ifndef COMPETITION_PHI_H
#define COMPETITION_PHI_H

#include "cangjie/CHIR/CHIR.h"
#include "cangjie/CHIR/IR/Package.h"
#include "cangjie/CHIR/IR/Value/Value.h"
#include "cangjie/Competition/RangeAnalysisSolver/Constraint.h"

#include <unordered_map>
#include <vector>


namespace Competition {

struct Alias {
    std::string def;
    int counter;

    Alias() : def(""), counter(0) {}
    Alias(std::string def) : def(def), counter(-1) {}

    void setCounter(int _counter) { counter = _counter; }

    const bool operator<(const Alias& other) const {
        return this->def < other.def
            || (this->def == other.def && this->counter < other.counter);
    }

    friend std::ostream& operator<<(std::ostream& os, const Alias& a) {
        os << a.def;
        if (a.counter != -1) os << "_" << a.counter;
        return os;
    }
};

class Phi {
private:
    Alias var;
    std::vector<Alias> aliases;
    size_t arity;

public:
    void addAlias(Alias other);
};

}


#endif // COMPETITION_PHI_H
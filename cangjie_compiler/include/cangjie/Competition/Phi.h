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
    Alias(std::string def, int counter) : def(def), counter(counter) {}

    // int foo() {
    //     return std::stoi(this->to_string().substr(def.find_last_of("_")+1, def.size()));
    // }
    int getCounter() { return counter; }
    void setCounter(int _counter) { counter = _counter; }

    const bool operator<(const Alias& other) const {
        return this->def < other.def
            || (this->def == other.def && this->counter < other.counter);
    }

    const bool operator==(const Alias& other) const {
        return this->def == other.def && this->counter == other.counter;
    }

    friend std::ostream& operator<<(std::ostream& os, const Alias& a) {
        os << a.def;
        if (a.counter != -1) os << "_" << a.counter;
        return os;
    }

    std::string to_string() {
        return def + "_" + std::to_string(counter);
    }
};

class Phi {
private:
    Alias var;
    std::vector<Alias> aliases;

public:
    Phi(Alias var, size_t arity) : var(var) {
        aliases = std::vector<Alias>(arity, var);
    }

    void addAlias(Alias other);
    void setAliasCounterByIdx(size_t idx, int counter) {
        aliases[idx].setCounter(counter);
    }

    void setVarCounter(int counter) {
        var.setCounter(counter);
    }

    std::string getVarDef() {
        return var.def;
    }

    std::string getAliasDefByIdx(size_t idx) {
        return aliases[idx].def;
    }

    friend std::ostream& operator<<(std::ostream& os, const Phi& a) {
        os << a.var << ": [";
        for (int i = 0; i < a.aliases.size(); i++) {
            if (i) os << ", ";
            os << a.aliases[i];
        }
        os << "]";
        return os;
    }
};

}


#endif // COMPETITION_PHI_H
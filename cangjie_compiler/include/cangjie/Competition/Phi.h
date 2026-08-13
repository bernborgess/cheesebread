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
    // Structure of the alias 
    /* Ex:

    func foo() {
        if (x > 0) {
            (before: x => Alias("foo","x",0) )
            x = 9; => Alias("foo","x",1)
        }
        return x;
    }

    * as a string:
          "foo:x:1"
            |  | | 
            |  | +----> count
            |  +------> identifier (GetSrcCodeIdentifier)
            +---------> func name, to avoid coliding between them.
            
    TODO: Parameters for all fields
    std::string funcName;
    std::string identifier;
    */ 
    int counter;

    Alias() : def(""), counter(0) {}
    Alias(std::string def) : def(def), counter(-1) {}
    Alias(std::string def, int counter) : def(def), counter(counter) {}

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
        if (a.counter != -1) os << ":" << a.counter;
        return os;
    }

    std::string to_string() {
        return def + ":" + std::to_string(counter);
    }

    static Alias from_string(std::string ssaName) {
        auto _pos = ssaName.find_last_of(':');
        return Alias(ssaName.substr(0, _pos), std::stoi(ssaName.substr(_pos + 1)));
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

    void addAlias(Alias other) {
        aliases.emplace_back(other);
    }

    void setAliasCounterByIdx(size_t idx, int counter) {
        aliases[idx].setCounter(counter);
    }

    void setVarCounter(int counter) {
        var.setCounter(counter);
    }

    Alias getVar() {
        return var;
    }

    std::string getVarDef() {
        return var.def;
    }

    int getVarCounter() {
        return var.counter;
    }

    std::string getAliasDefByIdx(size_t idx) {
        return aliases[idx].def;
    }

    std::string getVarString() {
        return var.to_string();
    }

    std::vector<std::string> getAliasesStrings() {
        std::vector<std::string> aliasesStrings;
        for (Alias a : aliases) {
            aliasesStrings.push_back(a.to_string());
        }
        return aliasesStrings;
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
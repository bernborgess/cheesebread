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
    // Structure of the alias 
    /* Ex:

    func foo() {
        var x = 10; => Alias("foo","x",0)
        if (x > 0) {
            x = 9; => Alias("foo","x",1)
        }
        return x;
    }

    * as a string:
          "foo:x:1"
            |  | | 
            |  | +----> count
            |  +------> identifier (GetSrcCodeIdentifier)
            +---------> funcName, to avoid coliding between them.
    */ 

    std::string funcName;
    std::string def;
    int counter;

    Alias() : funcName(""), def(""), counter(0) {}
    Alias(std::string funcName, std::string def)
        : funcName(funcName), def(def), counter(-1) {}
    Alias(std::string funcName, std::string def, int counter)
        : funcName(funcName), def(def), counter(counter) {}

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
        os << a.funcName << ":" << a.def << ":" << a.counter;
        return os;
    }

    std::string to_string() {
        return funcName + ":" + def + ":" + std::to_string(counter);
    }

    /// @brief Creates Alias from stringfied version
    /// @param ssaName "<funcName>:<def>:<counter>"
    static Alias from_string(std::string ssaName) {
        std::string funcName, def;
        int counter;
        std::stringstream ss(ssaName);

        std::getline(ss, funcName, ':');
        std::getline(ss, def, ':');
        ss >> counter;

        return Alias(funcName, def, counter);
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
        os << a.var << " φ[";
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
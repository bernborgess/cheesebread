#include "cangjie/Competition/Phi.h"

namespace Competition {
    void Phi::addAlias(Alias other) {
        // std::assert(other.def == var.def && "Variables must be equal");
        aliases.emplace_back(other);
    }
}
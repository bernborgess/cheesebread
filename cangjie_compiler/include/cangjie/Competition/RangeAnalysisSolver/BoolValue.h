#pragma once

#include "cangjie/Competition/RangeAnalysisSolver/AbstractValue.h"

class BoolValue : public AbstractValue<bool, 2> {
   public:
    void join(const BoolValue& other);
    void addConstant(const std::vector<bool>& vals) override;
    void setAsTop();

    /**
     * @brief Overload for printing the BoolValue state.
     * @attention Using the output format expected by the competition
     */
    friend std::ostream& operator<<(std::ostream& os, const BoolValue& av) {
        if (av.isBottom()) {
            os << "false, true";
            return os;
        }
        bool first = true;
        for (auto val : av.getValues()) {
            if (!first) os << ", ";
            os << (val ? "true" : "false");
            first = false;
        }
        return os;
    }
};
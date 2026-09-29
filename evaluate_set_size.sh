#!/bin/bash

#set -v
set -e

ROOT_DIR=$(pwd)
CONSTRAINT_H_PATH="$ROOT_DIR/cangjie_compiler/include/cangjie/Competition/RangeAnalysisSolver/Constraint.h"
if [ ! -f "$CONSTRAINT_H_PATH" ]; then
    echo "$CONSTRAINT_H_PATH does not exist."
    exit -1
fi

# Variate the set size 1,2,4,...,64
for i in {0..6}; do
    SET_SIZE=$((2**i))
    echo "Evaluating Set Size = ${SET_SIZE}"
    # Edit the macro
    vim $CONSTRAINT_H_PATH -s <(echo -e "/INT_VALUE_SET_SIZE/\nwcw${SET_SIZE}\e:wq")
    # Compile
    cd $ROOT_DIR
    FRESH=true ./setup.sh
    # Test
    cd test/CangjieBench
    ./compile.sh $SET_SIZE

    echo "results-${SET_SIZE}.csv should be ready."
done

echo "Done!"

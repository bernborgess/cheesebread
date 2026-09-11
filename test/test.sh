
TESTS=('array' 'call12' 'i-plus-j' 'i-plus-plus' 'official' 'readif' \
    'return-compare' 'return012' 'while10' 'x14' 'wcet')

CJC_PATH=../../cangjie_compiler/output/bin/
GENERATE_GRAPHS=true

for test in "${TESTS[@]}"; do
    echo -n "Test case '$test' "
    cd $test

    # Run the code
    $CJC_PATH/cjc code.cj -O2 -j1

    # Generate png of dot graphs
    if [ $GENERATE_GRAPHS = true ]; then
    for graph in *.dot; do
        # https://mywiki.wooledge.org/BashFAQ/073
        dot -Tpng "$graph" -o "${graph%.*}.png"
    done
    fi

    # Compare the output with expected
    if diff output.txt expected.txt; then
        echo "OK"
    else
        echo "FAIL"
        break
    fi

    cd ..
done


TESTS=('adpcm' 'bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn' \
    'expint' 'fac' 'fdct' 'fft1' 'fibcall' 'fir' 'insertsort' 'janne_complex' \
    'jfdctint' 'lcdnum' 'lms' 'ludcmp' 'matmult' 'minver' 'ndes' 'ns' \
    'nsichneu' 'prime' 'qsort-exam' 'qurt' 'recursion' 'select' 'sqrt' 'st' \
    'statemate' 'ud')

CJC_PATH=../../../cangjie_compiler/output/bin/

set -e

for test in "${TESTS[@]}"; do
    echo -n "Test case '$test' "
    cd $test

    # Create queries
    cat << EOF > input.txt
[$test.cj, 0, x]
EOF

    # Run the code
    $CJC_PATH/cjc $test.cj -O2 -j1 -Woff unused

    cd ..

    # Store the bitwidth reduction
    # ? After checked that they run ok

done

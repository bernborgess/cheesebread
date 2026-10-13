#TESTS=('adpcm' 'bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn' \
TESTS=('bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn' \
    'expint' 'fac' 'fdct' 'fft1' 'fibcall' 'fir' 'insertsort' 'janne_complex' \
    'jfdctint' 'lcdnum' 'lms' 'ludcmp' 'matmult' 'minver' 'ndes' 'ns' \
    'nsichneu' 'prime' 'qsort-exam' 'qurt' 'recursion' 'select' 'sqrt' 'st' \
    'statemate' 'ud')

set -e

# Will download all original C code from WCET

for test in "${TESTS[@]}"; do

    SRC="./$test/$test.c"
    DST="./$test/$test.cj"
    # Skip exisiting file
    [ -f "$DST" ] && continue
    echo -e "Test case '$test' "
    
    PROMPT="Translate the C code in file $SRC to Cangjie and store it to $DST"
    opencode run "$PROMPT"
done

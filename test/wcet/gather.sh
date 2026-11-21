TESTS=('adpcm' 'bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn' \
    'expint' 'fac' 'fdct' 'fft1' 'fibcall' 'fir' 'insertsort' 'janne_complex' \
    'jfdctint' 'lcdnum' 'lms' 'ludcmp' 'matmult' 'minver' 'ndes' 'ns' \
    'nsichneu' 'prime' 'qsort-exam' 'qurt' 'recursion' 'select' 'sqrt' 'st' \
    'statemate' 'ud')

PWD=$(pwd)

set -e

# Will download all original C code from WCET

for test in "${TESTS[@]}"; do
    echo -e "Test case '$test' "
    URL="https://www.mrtc.mdh.se/projects/wcet/wcet_bench/$test/$test.c"
    TARGET="$PWD/$test/$test.c"
    wget $URL -O $TARGET
done
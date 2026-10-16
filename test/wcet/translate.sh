TESTS=('adpcm' 'bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn' \
    'expint' 'fac' 'fdct' 'fft1' 'fibcall' 'fir' 'insertsort' 'janne_complex' \
    'jfdctint' 'lcdnum' 'lms' 'ludcmp' 'matmult' 'minver' 'ndes' 'ns' \
    'nsichneu' 'prime' 'qsort-exam' 'qurt' 'recursion' 'select' 'sqrt' 'st' \
    'statemate' 'ud')

mapfile -t MODELS < <( cat models.txt )
for test in "${TESTS[@]}"; do
    SRC="./$test/$test.c"
    DST="./$test/$test.cj"
    PROMPT="Translate the C code in file $SRC to Cangjie and store it to $DST"

    # Skip existing file
    [ -f "$DST" ] && continue

    echo -e "Test case '$test' "

    for MODEL in "${MODELS[@]}"; do
        echo "Translating $test with $MODEL..."
        opencode run -m "$MODEL" "$PROMPT"
        if [ $? -ne 0 ]; then
            echo "$MODEL failed."
        else
            [ -f "$DST" ] && break
        fi
    done
done


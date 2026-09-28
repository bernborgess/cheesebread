
INT_VALUE_SET_SIZE=64
RESULTS_FILE_NAME="results-${INT_VALUE_SET_SIZE}.csv"

source ../../cangjie_compiler/output/envsetup.sh

cjc -v

mkdir -p outs

echo "Storing results at ${RESULTS_FILE_NAME}"

# Reset the results file
cat << EOF > $RESULTS_FILE_NAME
TestCase,Number of Variables,Bitwidth Reduction,Analysis Running Time
EOF

# Will attempt to compile each file, report errors if failed.
#for testcase in *.cj; do
for i in {0..163}; do
    echo -n "$i.cj," >> $RESULTS_FILE_NAME

    cjc $i.cj -o outs -Woff unused -O2 -j1 --output-type=staticlib
    if [ $? -ne 0 ]; then
        echo "FAILED at $i.cj"
        exit -1
    else
        echo "PASS $i.cj";
    fi

done

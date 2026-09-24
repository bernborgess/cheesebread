
source ../../cangjie_compiler/output/envsetup.sh

cjc -v

mkdir -p outs

# Reset the results file
cat << EOF > results.csv
TestCase,Number of Variables,Bitwidth Reduction,Analysis Running Time
EOF

# Will attempt to compile each file, report errors if failed.
#for testcase in *.cj; do
for i in {0..163}; do
    echo -n "$i.cj," >> results.csv

    cjc $i.cj -o outs -Woff unused -O2 -j1 --output-type=staticlib
    if [ $? -ne 0 ]; then
        echo "FAILED at $i.cj"
        exit -1
    else
        echo "PASS $i.cj";
    fi

done
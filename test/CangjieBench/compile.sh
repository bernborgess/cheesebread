
source ../../cangjie_compiler/output/envsetup.sh

cjc -v

mkdir -p outs

# Will attempt to compile each file, report errors if failed.
#for testcase in *.cj; do
for i in {0..163}; do
    echo -n "$i.cj," >> results.csv

    start=`date +%s.%N`

    cjc $i.cj -o outs -Woff unused -O2 -j1 --output-type=staticlib
    if [ $? -ne 0 ]; then
        echo "FAILED at $i.cj"
        exit -1
    else
        echo "PASS $i.cj";
    fi

    end=`date +%s.%N`
    runtime=$( echo "$end - $start" | bc -l )
    echo "$runtime" >> results.csv

done
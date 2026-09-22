
source ../../cangjie_compiler/output/envsetup.sh

cjc -v

mkdir -p outs

# Will attempt to compile each file, report errors if failed.
#for testcase in *.cj; do
for i in {0..163}; do
    cjc "$i.cj" -o outs/$i -Woff unused
    if [ $? -ne 0 ]; then
        echo "FAILED at $i.cj"
        exit -1
    else
        echo "PASS $i.cj";
    fi
done
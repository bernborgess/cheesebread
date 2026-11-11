/* MDH WCET BENCHMARK SUITE. File version $Id: duff.c 7800 2018-01-19 12:43:12Z lkg02 $ */

let ARRAYSIZE: Int64 = 100
let INVOCATION_COUNT: Int64 = 43

func duffcopy(to: Array<Int64>, from: Array<Int64>, count: Int64) {
    var n: Int64 = (count + 7) / 8
    var ti: Int64 = 0
    var fi: Int64 = 0

    var rem: Int64 = count % 8
    if (rem == 0) {
        rem = 8
    }

    var k: Int64 = 0
    // @range-cat: arith-bound - remainder of count % 8 is at most 7, bumped to 8 for exact multiples
    sassert(rem <= 8)
    while (k < rem) {
        to[ti + k] = from[fi + k]
        k = k + 1
    }
    ti = ti + rem
    fi = fi + rem
    n = n - 1

    // @range-cat: arith-bound - copy offset after the prologue is bounded by the remainder
    sassert(ti <= 43)
    while (n > 0) {
        var j: Int64 = 0
        while (j < 8) {
            // @range-cat: array-bound - Duff's inner counter is bounded by the unrolled factor 8
            sassert(j <= 7)
            to[ti + j] = from[fi + j]
            j = j + 1
        }
        ti = ti + 8
        fi = fi + 8
        n = n - 1
    }
}

func initialize(arr: Array<Int64>, length: Int64) {
    var i: Int64 = 0
    while (i < length) {
        // @range-cat: array-bound - index is bounded by the array length before the store
        sassert(i <= 99)
        // @range-cat: branch-bound - length is the constant ARRAYSIZE = 100 at every call site
        sassert(length <= 100)
        arr[i] = length - i
        i = i + 1
    }
}

var source: Array<Int64> = Array<Int64>(ARRAYSIZE, repeat: 0)
var target: Array<Int64> = Array<Int64>(ARRAYSIZE, repeat: 0)

main() {
    initialize(source, ARRAYSIZE)
    duffcopy(source, target, INVOCATION_COUNT)
}

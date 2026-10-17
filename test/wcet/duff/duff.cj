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
    while (k < rem) {
        to[ti + k] = from[fi + k]
        k = k + 1
    }
    ti = ti + rem
    fi = fi + rem
    n = n - 1

    while (n > 0) {
        var j: Int64 = 0
        while (j < 8) {
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

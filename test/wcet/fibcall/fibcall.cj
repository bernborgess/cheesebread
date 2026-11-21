/* MDH WCET BENCHMARK SUITE. File version $Id: fibcall.c 7800 2018-01-19 12:43:12Z lkg02 $ */

func fib(n: Int64): Int64 {
    var i: Int64 = 0
    var Fnew: Int64 = 0
    var Fold: Int64 = 0
    var temp: Int64 = 0
    var ans: Int64 = 0

    Fnew = 1
    Fold = 0
    i = 2
    while (i <= n) {
        temp = Fnew
        Fnew = Fnew + Fold
        Fold = temp
        i = i + 1
    }
    ans = Fnew
    return ans
}

main() {
    var a: Int64 = 0

    a = 30
    fib(a)
    return a
}
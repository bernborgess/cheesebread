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
        // @range-cat: loop-bound - guard i <= n with n bound to 30 by the unique call fib(30)
        sassert(i <= 30)
        // @range-cat: branch-bound - n is the parameter of the only call site, fixed to 30
        sassert(n == 30)
        // @range-cat: arith-bound - Fibonacci values are non-decreasing and start at 1
        sassert(Fnew >= 1)
        // @range-cat: arith-bound - previous Fibonacci value is always non-negative
        sassert(Fold >= 0)
        temp = Fnew
        Fnew = Fnew + Fold
        Fold = temp
        i = i + 1
    }
    // @range-cat: loop-bound - after the loop i has advanced one past n = 30
    sassert(i <= 31)
    ans = Fnew
    // @range-cat: arith-bound - fib(30) = 832040 is well below the two-million cap
    sassert(ans <= 2000000)
    return ans
}

main() {
    var a: Int64 = 0

    a = 30
    fib(a)
    return a
}
/* MDH WCET BENCHMARK SUITE. File version $Id: fac.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/*
 * Changes: CS 2006/05/19: Changed loop bound from constant to variable.
 */

func fac(n: Int64): Int64 {
    if (n == 0) {
        return 1
    } else {
        // @range-cat: loop-bound - recursion only descends while the argument stays within [0,5]
        sassert(n <= 5)
        return n * fac(n - 1)
    }
}

main(): Int64 {
    var i: Int64 = 0
    var s: Int64 = 0
    var n: Int64 = 5

    while (i <= n) {
        // @range-cat: loop-bound - loop guard i <= n with n fixed at 5 keeps i within [0,5]
        sassert(i <= 5)
        // @range-cat: branch-bound - n is initialised to the constant 5 and never modified
        sassert(n <= 5)
        s += fac(i)
        i = i + 1
    }

    // @range-cat: loop-bound - the loop can only exit once i has passed the constant n
    sassert(i <= 6)
    return s
}

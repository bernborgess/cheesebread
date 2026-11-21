/* MDH WCET BENCHMARK SUITE. File version $Id: fac.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/*
 * Changes: CS 2006/05/19: Changed loop bound from constant to variable.
 */

func fac(n: Int64): Int64 {
    if (n == 0) {
        return 1
    } else {
        return n * fac(n - 1)
    }
}

main(): Int64 {
    var i: Int64 = 0
    var s: Int64 = 0
    var n: Int64 = 5

    while (i <= n) {
        s += fac(i)
        i = i + 1
    }

    return s
}

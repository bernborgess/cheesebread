/* MDH WCET BENCHMARK SUITE. File version $Id: recursion.c 7800 2018-01-19 12:43:12Z lkg02 $ */

/* Generate an example of recursive code, to see  *
 * how it can be modeled in the scope graph.      */

/* self-recursion  */
func fib(i: Int64): Int64 {
    if (i == 0) {
        return 1
    }
    if (i == 1) {
        return 1
    }
    // @range-cat: branch-bound - reaching here means i != 0 and i != 1, hence i >= 2
    sassert(i >= 2)
    // @range-cat: branch-bound - the i == 0 case returned earlier, so zero is excluded
    sassert(i != 0)
    // @range-cat: loop-bound - recursion depth stays bounded by the initial argument 10
    sassert(i <= 10)
    return fib(i - 1) + fib(i - 2)
}

/* mutual recursion */
func kalle(j: Int64): Int64 {
    if (j <= 0) {
        return 0
    } else {
        // @range-cat: branch-bound - the false branch of j <= 0 constrains j to be positive
        sassert(j >= 1)
        return anka(j - 1)
    }
}

func anka(k: Int64): Int64 {
    if (k <= 0) {
        return 1
    } else {
        // @range-cat: branch-bound - the false branch of k <= 0 constrains k to be positive
        sassert(k >= 1)
        return kalle(k - 1)
    }
}

var In: Int64 = 0

main(): Int64 {
    kalle(10)
    In = fib(10)
    return 0
}

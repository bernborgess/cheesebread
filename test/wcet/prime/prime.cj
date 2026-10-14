/* MDH WCET BENCHMARK SUITE. File version $Id: prime.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* Changes:
 * JG 2005/12/08: Prototypes added, and changed exit to return in main.
 */

typedef UInt = UInt64

func divides(n: UInt, m: UInt) -> Bool {
    return (m % n == 0)
}

func even(n: UInt) -> Bool {
    return (divides(2, n))
}

func prime(n: UInt) -> Bool {
    var i: UInt = 0
    if (even(n)) {
        return (n == 2)
    }
    i = 3
    while (i * i <= n) {
        if (divides(i, n)) { /* ai: loop here min 0 max 357 end; */
            return false
        }
        i += 2
    }
    return (n > 1)
}

func swap(a: CPointer<UInt>, b: CPointer<UInt>) {
    let tmp = *a
    *a = *b
    *b = tmp
}

func main() -> Int64 {
    var x: UInt = 21649
    var y: UInt = 513239
    swap(&x, &y)
    return (!((prime(x) && prime(y))) as Int64)
}

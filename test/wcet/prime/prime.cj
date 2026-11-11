/* MDH WCET BENCHMARK SUITE. File version $Id: prime.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* Changes:
 * JG 2005/12/08: Prototypes added, and changed exit to return in main.
 */

/* The C typedef `bool` is `unsigned char`; the comparison results below map
 * directly onto Cangjie's native `Bool`. The C typedef `uint` is 32 bit on the
 * target, so `UInt32` is used to reproduce its width and wraparound behaviour.
 */

func divides(n: UInt32, m: UInt32): Bool {
    return m % n == 0
}

func even(n: UInt32): Bool {
    return divides(2, n)
}

func prime(n: UInt32): Bool {
    var i: UInt32 = 0
    // @range-cat: branch-bound - the only call sites pass the constants 21649 and 513239
    sassert(n <= 513239)
    // @range-cat: branch-bound - 21649 is the smaller of the two call-site constants
    sassert(n >= 21649)
    // @range-cat: loop-bound - the trial-division induction variable starts at 0
    sassert(i <= 0)
    if (even(n)) {
        return n == 2
    }
    i = 3
    // @range-cat: arith-bound - the trial divisor scan starts at the constant 3
    sassert(i == 3u32)
    while (i * i <= n) {
        /* ai: loop here min 0 max 357 end; */
        // @range-cat: loop-bound - i only ever grows from its initial value 3
        sassert(i >= 3)
        // @range-cat: branch-bound - both call-site arguments are non-zero
        sassert(n != 0)
        if (divides(i, n)) {
            return false
        }
        i = i + 2
    }
    // @range-cat: arith-bound - the largest call-site constant is 513239
    sassert(n <= 513239u32)
    return n > 1
}

/* `swap` takes pointers to `uint` in C. A Cangjie reference type is used to
 * give the function the same read/write access to the caller's variables. */
class UInt32Box {
    var value: UInt32

    public init(value: UInt32) {
        this.value = value
    }
}

func swap(a: UInt32Box, b: UInt32Box) {
    let tmp: UInt32 = a.value
    a.value = b.value
    b.value = tmp
}

main(): Int64 {
    var x: UInt32Box = UInt32Box(21649)
    var y: UInt32Box = UInt32Box(513239)

    swap(x, y)

    if (!(prime(x.value) && prime(y.value))) {
        return 1
    }
    return 0
}

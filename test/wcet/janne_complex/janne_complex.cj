/* MDH WCET BENCHMARK SUITE. File version $Id: janne_complex.c 7800 2018-01-19 12:43:12Z lkg02 $ */

func complex(a0: Int64, b0: Int64): Int64 {
    var a: Int64 = a0
    var b: Int64 = b0

    while (a < 30) {
        // @range-cat: loop-bound - outer guard keeps the induction variable below 30
        sassert(a <= 29)
        while (b < a) {
            // @range-cat: loop-bound - inner guard b < a <= 29 keeps b below 30
            sassert(b <= 29)
            if (b > 5) {
                // @range-cat: branch-bound - true branch of b > 5 raises the lower bound to 6
                sassert(b >= 6)
                b = b * 3
            } else {
                // @range-cat: branch-bound - false branch of b > 5 caps b at 5
                sassert(b <= 5)
                b = b + 2
            }
            if (b >= 10 && b <= 12) {
                a = a + 10
            } else {
                a = a + 1
            }
        }
        a = a + 2
        b = b - 10
        // @range-cat: loop-bound - a only ever grows from its initial value 1
        sassert(a >= 1)
    }
    return 1
}

main() {
    /* a = [1..30] b = [1..30] */
    var a: Int64 = 1
    var b: Int64 = 1
    var answer: Int64 = 0
    /* if(answer)
       {a = 1; b = 1;}
       else
       {a = 30; b = 30;} */
    answer = complex(a, b)
    return answer
}

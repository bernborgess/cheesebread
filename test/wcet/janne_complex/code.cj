/* MDH WCET BENCHMARK SUITE. File version $Id: janne_complex.c 7800 2018-01-19 12:43:12Z lkg02 $ */

func complex(a0: Int64, b0: Int64): Int64 {
    var a: Int64 = a0
    var b: Int64 = b0

    while (a < 30) {
        while (b < a) {
            if (b > 5) {
                b = b * 3
            } else {
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

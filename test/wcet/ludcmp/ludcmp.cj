/* MDH WCET BENCHMARK SUITE. File version $Id: ludcmp.c 7800 2018-01-19 12:43:12Z lkg02 $ */

/*************************************************************************/
/*                                                                       */
/*   SNU-RT Benchmark Suite for Worst Case Timing Analysis               */
/*   =====================================================               */
/*                              Collected and Modified by S.-S. Lim      */
/*                                           sslim@archi.snu.ac.kr       */
/*                                         Real-Time Research Group      */
/*                                        Seoul National University      */
/*                                                                       */
/*                                                                       */
/*        < Features > - restrictions for our experimental environment   */
/*                                                                       */
/*          1. Completely structured.                                    */
/*               - There are no unconditional jumps.                     */
/*               - There are no exit from loop bodies.                   */
/*                 (There are no 'break' or 'return' in loop bodies)     */
/*          2. No 'switch' statements.                                   */
/*          3. No 'do..while' statements.                                */
/*          4. Expressions are restricted.                               */
/*               - There are no multiple expressions joined by 'or',     */
/*                'and' operations.                                      */
/*          5. No library calls.                                         */
/*               - All the functions needed are implemented in the       */
/*                 source file.                                          */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: ludcmp.c                                                       */
/*  SOURCE : Turbo C Programming for Engineering                         */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Simultaneous linear equations by LU decomposition.                */
/*     The arrays a[][] and b[] are input and the array x[] is output    */
/*     row vector.                                                       */
/*     The variable n is the number of equations.                        */
/*     The input arrays are initialized in function main.                */
/*                                                                       */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/


/* Changes:
 * JG 2005/12/12: Indented program. Removed unused variable nmax.
 */

/*
** Benchmark Suite for Real-Time Applications, by Sung-Soo Lim
**
**    III-4. ludcmp.c : Simultaneous Linear Equations by LU Decomposition
**                 (from the book C Programming for EEs by Hyun Soon Ahn)
*/



var a: Array<Array<Float64>> = Array<Array<Float64>>(50, repeat: Array<Float64>(50, repeat: 0.0))
var b: Array<Float64> = Array<Float64>(50, repeat: 0.0)
var x: Array<Float64> = Array<Float64>(50, repeat: 0.0)

/* Cangjie's `Array(size, repeat: ...)` shares the same inner array between rows,
   so the rows of `a` are replaced by independent arrays at the start of main. */
func initMatrix() {
    var r: Int64 = 0
    while (r < 50) {
        a[r] = Array<Float64>(50, repeat: 0.0)
        r = r + 1
    }
}

func fabs(n: Float64): Float64 {
    var f: Float64 = 0.0

    if (n >= 0.0) {
        f = n
    } else {
        f = -n
    }
    return f
}

main(): Int64 {

    var i: Int64 = 0
    var j: Int64 = 0
    var n: Int64 = 5
    var chkerr: Int64 = 0
    var eps: Float64 = 0.0
    var w: Float64 = 0.0

    initMatrix()

    eps = 1.0e-6

    i = 0
    while (i <= n) {
        w = 0.0
        j = 0
        while (j <= n) {
            a[i][j] = Float64((i + 1) + (j + 1))
            if (i == j) {
                a[i][j] = a[i][j] * 10.0
            }
            w = w + a[i][j]
            j = j + 1
        }
        b[i] = w
        i = i + 1
    }

    chkerr = ludcmp(n, eps)

    return 0

}

func ludcmp(n: Int64, eps: Float64): Int64 {

    var i: Int64 = 0
    var j: Int64 = 0
    var k: Int64 = 0
    var w: Float64 = 0.0
    var y: Array<Float64> = Array<Float64>(100, repeat: 0.0)

    if (n > 99 || eps <= 0.0) {
        return 999
    }
    i = 0
    while (i < n) {
        if (fabs(a[i][i]) <= eps) {
            return 1
        }
        j = i + 1
        while (j <= n) {
            w = a[j][i]
            if (i != 0) {
                k = 0
                while (k < i) {
                    w = w - a[j][k] * a[k][i]
                    k = k + 1
                }
            }
            a[j][i] = w / a[i][i]
            j = j + 1
        }
        j = i + 1
        while (j <= n) {
            w = a[i + 1][j]
            k = 0
            while (k <= i) {
                w = w - a[i + 1][k] * a[k][j]
                k = k + 1
            }
            a[i + 1][j] = w
            j = j + 1
        }
        i = i + 1
    }
    y[0] = b[0]
    i = 1
    while (i <= n) {
        w = b[i]
        j = 0
        while (j < i) {
            w = w - a[i][j] * y[j]
            j = j + 1
        }
        y[i] = w
        i = i + 1
    }
    x[n] = y[n] / a[n][n]
    i = n - 1
    while (i >= 0) {
        w = y[i]
        j = i + 1
        while (j <= n) {
            w = w - a[i][j] * x[j]
            j = j + 1
        }
        x[i] = w / a[i][i]
        i = i - 1
    }
    return 0

}

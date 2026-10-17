/* MDH WCET BENCHMARK SUITE. File version $Id: minver.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* 2012/09/28, Jan Gustafsson <jan.gustafsson@mdh.se>
 * Changes:
 *  - Missing braces around initialization of subobject added
 *  - This program redefines the standard C function fabs. Therefore, the
 *  function has been renamed to minver_fabs.
 */

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
/*  FILE: minver.c                                                       */
/*  SOURCE : Turbo C Programming for Engineering by Hyun Soo Ahn         */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Matrix inversion for 3x3 floating point matrix.                   */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/


// func minver(row: Int64, col: Int64, eps: Float64): Int64
// func mmul(row_a: Int64, col_a: Int64, row_b: Int64, col_b: Int64): Int64

let a: Array<Array<Float64>> = [
    [3.0, -6.0, 7.0],
    [9.0, 0.0, -5.0],
    [5.0, -8.0, 6.0]
]
var b: Array<Array<Float64>> = Array<Array<Float64>>(3, {_ => Array<Float64>(3, repeat: 0.0)})
var c: Array<Array<Float64>> = Array<Array<Float64>>(3, {_ => Array<Float64>(3, repeat: 0.0)})
var aa: Array<Array<Float64>> = Array<Array<Float64>>(3, {_ => Array<Float64>(3, repeat: 0.0)})
var a_i: Array<Array<Float64>> = Array<Array<Float64>>(3, {_ => Array<Float64>(3, repeat: 0.0)})
var e: Array<Array<Float64>> = Array<Array<Float64>>(3, {_ => Array<Float64>(3, repeat: 0.0)})
var det: Float64 = 0.0

func minver_fabs(n: Float64): Float64 {
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
    var eps: Float64 = 0.0

    eps = 1.0e-6

    i = 0
    while (i < 3) {
        j = 0
        while (j < 3) {
            aa[i][j] = a[i][j]
            j = j + 1
        }
        i = i + 1
    }

    minver(3, 3, eps)
    i = 0
    while (i < 3) {
        j = 0
        while (j < 3) {
            a_i[i][j] = a[i][j]
            j = j + 1
        }
        i = i + 1
    }

    mmul(3, 3, 3, 3)
    return 0
}

func mmul(row_a: Int64, col_a: Int64, row_b: Int64, col_b: Int64): Int64 {
    var i: Int64 = 0
    var j: Int64 = 0
    var k: Int64 = 0
    var row_c: Int64 = 0
    var col_c: Int64 = 0
    var w: Float64 = 0.0

    row_c = row_a
    col_c = col_b

    if (row_c < 1 || row_b < 1 || col_c < 1 || col_a != row_b) {
        return 999
    }
    i = 0
    while (i < row_c) {
        j = 0
        while (j < col_c) {
            w = 0.0
            k = 0
            while (k < row_b) {
                w = w + a[i][k] * b[k][j]
                k = k + 1
            }
            c[i][j] = w
            j = j + 1
        }
        i = i + 1
    }
    return 0
}

func minver(row: Int64, col: Int64, eps: Float64): Int64 {
    var work: Array<Int64> = Array<Int64>(500, repeat: 0)
    var i: Int64 = 0
    var j: Int64 = 0
    var k: Int64 = 0
    var r: Int64 = 0
    var iw: Int64 = 0
    var s: Int64 = 0
    var t: Int64 = 0
    var u: Int64 = 0
    var v: Int64 = 0
    var w: Float64 = 0.0
    var wmax: Float64 = 0.0
    var pivot: Float64 = 0.0
    var api: Float64 = 0.0
    var w1: Float64 = 0.0

    if (row < 2 || row > 500 || eps <= 0.0) {
        return 999
    }
    w1 = 1.0
    i = 0
    while (i < row) {
        work[i] = i
        i = i + 1
    }
    k = 0
    while (k < row) {
        wmax = 0.0
        i = k
        while (i < row) {
            w = minver_fabs(a[i][k])
            if (w > wmax) {
                wmax = w
                r = i
            }
            i = i + 1
        }
        pivot = a[r][k]
        api = minver_fabs(pivot)
        if (api <= eps) {
            det = w1
            return 1
        }
        w1 = w1 * pivot
        u = k * col
        v = r * col
        if (r != k) {
            w1 = -w
            iw = work[k]
            work[k] = work[r]
            work[r] = iw
            j = 0
            while (j < row) {
                s = u + j
                t = v + j
                w = a[k][j]
                a[k][j] = a[r][j]
                a[r][j] = w
                j = j + 1
            }
        }
        i = 0
        while (i < row) {
            a[k][i] = a[k][i] / pivot
            i = i + 1
        }
        i = 0
        while (i < row) {
            if (i != k) {
                v = i * col
                s = v + k
                w = a[i][k]
                if (w != 0.0) {
                    j = 0
                    while (j < row) {
                        if (j != k) {
                            a[i][j] = a[i][j] - w * a[k][j]
                        }
                        j = j + 1
                    }
                    a[i][k] = -w / pivot
                }
            }
            i = i + 1
        }
        a[k][k] = 1.0 / pivot
        k = k + 1
    }
    i = 0
    while (i < row) {
        while (true) {
            k = work[i]
            if (k == i) {
                break
            }
            iw = work[k]
            work[k] = work[i]
            work[i] = iw
            j = 0
            while (j < row) {
                u = j * col
                s = u + i
                t = u + k
                w = a[k][i]
                a[k][i] = a[k][k]
                a[k][k] = w
                j = j + 1
            }
        }
        i = i + 1
    }
    det = w1
    return 0
}
/* MDH WCET BENCHMARK SUITE. File version $Id: ud.c 7801 2018-01-19 13:33:32Z lkg02 $ */

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

/*************************************************************************
 *  This file:
 *
 *  - Name changed to "ud.c"
 *  - Modified for use with Uppsala/Paderborn tool
 *    : doubles changed to int
 *    : some tests removed
 *  - Program is much more linear, all loops will run to end
 *  - Purpose: test the effect of conditional flows
 *
 *************************************************************************/

/*
** Benchmark Suite for Real-Time Applications, by Sung-Soo Lim
**
**    III-4. ludcmp.c : Simultaneous Linear Equations by LU Decomposition
**                 (from the book C Programming for EEs by Hyun Soon Ahn)
*/

var a: Array<Array<Int64>> = Array<Array<Int64>>(50, {_ => Array<Int64>(50, repeat: 0)})
var b: Array<Int64> = Array<Int64>(50, repeat: 0)
var x: Array<Int64> = Array<Int64>(50, repeat: 0)

func ludcmp(nmax: Int64, n: Int64): Int64 {

    var i: Int64 = 0
    var j: Int64 = 0
    var k: Int64 = 0
    var w: Int64 = 0
    var y: Array<Int64> = Array<Int64>(100, repeat: 0)

    i = 0
    while (i < n) {
        /* if(fabs(a[i][i]) <= eps) return(1); */
        j = i + 1
        while (j <= n) {    /* triangular loop vs. i */
            w = a[j][i]
            if (i != 0) {   /* sub-loop is conditional, done
                               all iterations except first of the
                               OUTER loop */
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
        while (j <= n) {    /* triangular loop vs. i */
            w = a[i + 1][j]
            k = 0
            while (k <= i) {    /* triangular loop vs. i */
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
    while (i <= n) {        /* iterates n times */
        w = b[i]
        j = 0
        while (j < i) {     /* triangular sub loop */
            w = w - a[i][j] * y[j]
            j = j + 1
        }
        y[i] = w
        i = i + 1
    }
    x[n] = y[n] / a[n][n]
    i = n - 1
    while (i >= 0) {        /* iterates n times */
        w = y[i]
        j = i + 1
        while (j <= n) {    /* triangular sub loop */
            w = w - a[i][j] * x[j]
            j = j + 1
        }
        x[i] = w / a[i][i]
        i = i - 1
    }
    return 0

}

main(): Int64 {

    var i: Int64 = 0
    var j: Int64 = 0
    var nmax: Int64 = 50
    var n: Int64 = 5
    var chkerr: Int64 = 0
    var w: Int64 = 0

    /* eps = 1.0e-6; */

    /* Init loop */
    i = 0
    while (i <= n) {
        w = 0                   /* data to fill in cells */
        j = 0
        while (j <= n) {
            a[i][j] = (i + 1) + (j + 1)
            if (i == j) {       /* only once per loop pass */
                a[i][j] = a[i][j] * 2
            }
            w = w + a[i][j]
            j = j + 1
        }
        b[i] = w
        i = i + 1
    }

    /*  chkerr = ludcmp(nmax, n, eps); */
    chkerr = ludcmp(nmax, n)
    return 0

}
